/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "inspector.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#include <functional>
#include <imgui.h>

#include <editor/app.h>
#include <editor/gui/transform_inspector.h>
#include <editor/gui/pvar_inspector.h>

static u32 get_invaliation_id(const Instances& instances);

struct InspectorFieldFuncs
{
	s32 lane_count;
	std::function<bool(Instance& lhs, Instance& rhs, s32 lane)> compare;
	std::function<void(Level& lvl, Instance& first, bool values_equal[MAX_LANES])> draw;
};

struct InspectorField
{
	InstanceComponent required_component;
	InstanceType required_type;
	const char* name;
	InspectorFieldFuncs funcs;
};

static void draw_fields(Level& lvl, const std::vector<InspectorField>& fields);

template <typename Value>
struct InspectorGetterSetter
{
	std::function<Value(Instance& inst)> get;
	std::function<void(Instance& inst, Value value)> set;
};

static InspectorFieldFuncs type_funcs();
static InspectorFieldFuncs id_funcs();
template <typename Value>
static InspectorFieldFuncs scalar_funcs(InspectorGetterSetter<Value> getset);
static InspectorFieldFuncs bool_funcs(InspectorGetterSetter<bool> getset);
static InspectorFieldFuncs vec3_funcs(InspectorGetterSetter<glm::vec3> getset);
static InspectorFieldFuncs vec4_funcs(InspectorGetterSetter<glm::vec4> getset);
template <typename ThisInstance>
static InspectorFieldFuncs foreign_id_funcs(InstanceType foreign_type, s32 ThisInstance::*field);
static InspectorFieldFuncs camera_collision_funcs();
static InspectorFieldFuncs moby_rooted_funcs();

static bool should_draw_field(Level& lvl, const InspectorField& field);
static void should_draw_current_values(
	bool values_equal[MAX_LANES], Level& lvl, const InspectorField& field);

template <s32 lane_count, typename Value>
static void apply_to_all_selected(Level& lvl, Value value, std::array<bool, MAX_LANES> lanes, InspectorGetterSetter<Value> funcs);
template <typename Value>
static InspectorGetterSetter<Value> adapt_getter_setter(
	Value (Instance::*getter)() const, void (Instance::*setter)(Value));
template <typename Value>
static InspectorGetterSetter<Value> adapt_reference_member_function(
	Value& (Instance::*member_function)());
template <typename Value, typename ThisInstance>
static InspectorGetterSetter<Value> adapt_member_pointer(Value ThisInstance::*member_pointer);

static float calc_remaining_item_width();
static std::array<std::string, MAX_LANES> vec4_to_strings(glm::vec4 vec, bool values_equal[MAX_LANES]);
static Opt<glm::vec4> strings_to_vec4(
	std::array<std::string, MAX_LANES>& strings, std::array<bool, MAX_LANES>& changed);
template <typename Scalar>
static Opt<Scalar> string_to_scalar(std::string& string);

void inspector()
{
	app& a = *g_app;
	if (!a.get_level()) {
		ImGui::Text("<no level>");
		return;
	}
	Level& lvl = *a.get_level();
	
	static const std::vector<InspectorField> header_fields = {
		{COM_NONE            , INST_NONE      , "Type     ", type_funcs()},
		{COM_NONE            , INST_NONE      , "ID       ", id_funcs()},
	};
	
	static const std::vector<InspectorField> fields = {
		{COM_COLOUR          , INST_NONE          , "Colour", vec3_funcs(adapt_reference_member_function<glm::vec3>(&Instance::colour))},
		{COM_DRAW_DISTANCE   , INST_NONE          , "Draw Distance", scalar_funcs(adapt_reference_member_function<f32>(&Instance::draw_distance))},
		{COM_CAMERA_COLLISION, INST_NONE          , "Cam Coll", camera_collision_funcs()},
		// Moby
		{COM_NONE            , INST_MOBY          , "Mission", scalar_funcs(adapt_member_pointer(&MobyInstance::mission))},
		{COM_NONE            , INST_MOBY          , "UID", scalar_funcs(adapt_member_pointer(&MobyInstance::uid))},
		{COM_NONE            , INST_MOBY          , "Bolts", scalar_funcs(adapt_member_pointer(&MobyInstance::bolts))},
		{COM_NONE            , INST_MOBY          , "Class", scalar_funcs(adapt_reference_member_function<s32>(&MobyInstance::o_class))},
		{COM_NONE            , INST_MOBY          , "Update Distance", scalar_funcs(adapt_member_pointer(&MobyInstance::update_distance))},
		{COM_NONE            , INST_MOBY          , "Rooted", moby_rooted_funcs()},
		{COM_NONE            , INST_MOBY          , "Occlusion", scalar_funcs(adapt_member_pointer(&MobyInstance::occlusion))},
		{COM_NONE            , INST_MOBY          , "Mode Bits", scalar_funcs(adapt_member_pointer(&MobyInstance::mode_bits))},
		{COM_NONE            , INST_MOBY          , "Light", foreign_id_funcs(INST_DIRLIGHT, &MobyInstance::light)},
		{COM_NONE            , INST_MOBY          , "Static Collision", bool_funcs(adapt_member_pointer(&MobyInstance::has_static_collision))},
		// Tie
		{COM_NONE            , INST_TIE           , "Class", scalar_funcs(adapt_reference_member_function<s32>(&TieInstance::o_class))},
		{COM_NONE            , INST_TIE           , "Occlusion", scalar_funcs(adapt_member_pointer(&TieInstance::occlusion_index))},
		{COM_NONE            , INST_TIE           , "Light", foreign_id_funcs(INST_DIRLIGHT, &TieInstance::directional_lights)},
		{COM_NONE            , INST_TIE           , "UID", scalar_funcs(adapt_member_pointer(&TieInstance::uid))},
		{COM_NONE            , INST_TIE           , "Static Collision", bool_funcs(adapt_member_pointer(&TieInstance::has_static_collision))},
		// Shrub
		{COM_NONE            , INST_SHRUB         , "Class", scalar_funcs(adapt_reference_member_function<s32>(&ShrubInstance::o_class))},
		{COM_NONE            , INST_SHRUB         , "Unk 5c", scalar_funcs(adapt_member_pointer(&ShrubInstance::unknown_5c))},
		{COM_NONE            , INST_SHRUB         , "Dir Lights", scalar_funcs(adapt_member_pointer(&ShrubInstance::dir_lights))},
		{COM_NONE            , INST_SHRUB         , "Unk 64", scalar_funcs(adapt_member_pointer(&ShrubInstance::unknown_64))},
		{COM_NONE            , INST_SHRUB         , "Unk 68", scalar_funcs(adapt_member_pointer(&ShrubInstance::unknown_68))},
		{COM_NONE            , INST_SHRUB         , "Unk 6c", scalar_funcs(adapt_member_pointer(&ShrubInstance::unknown_6c))},
		{COM_NONE            , INST_SHRUB         , "Static Collision", bool_funcs(adapt_member_pointer(&ShrubInstance::has_static_collision))},
		// DirLight
		{COM_NONE            , INST_DIRLIGHT      , "Colour A", vec4_funcs(adapt_member_pointer(&DirLightInstance::col_a))},
		{COM_NONE            , INST_DIRLIGHT      , "Direction A", vec4_funcs(adapt_member_pointer(&DirLightInstance::dir_a))},
		{COM_NONE            , INST_DIRLIGHT      , "Colour B", vec4_funcs(adapt_member_pointer(&DirLightInstance::col_b))},
		{COM_NONE            , INST_DIRLIGHT      , "Direction B", vec4_funcs(adapt_member_pointer(&DirLightInstance::dir_b))},
		// PointLight
		{COM_NONE            , INST_POINTLIGHT    , "Radius", scalar_funcs(adapt_member_pointer(&PointLightInstance::radius))},
		// EnvSamplePoint
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Hero Light", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::hero_light))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Music Track", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::music_track))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Hero Colour", vec3_funcs(adapt_member_pointer(&EnvSamplePointInstance::hero_col))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Enable Reverb Params", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::enable_reverb_params))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Reverb Type", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::reverb_type))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Reverb Depth", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::reverb_depth))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Reverb Delay", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::reverb_delay))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Reverb Feedback", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::reverb_feedback))},
		// EnvTransition
		{COM_NONE            , INST_ENVTRANSITION , "Enable Hero", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::enable_hero))},
		{COM_NONE            , INST_ENVTRANSITION , "Hero Colour 1", vec3_funcs(adapt_member_pointer(&EnvTransitionInstance::hero_col_1))},
		{COM_NONE            , INST_ENVTRANSITION , "Hero Colour 2", vec3_funcs(adapt_member_pointer(&EnvTransitionInstance::hero_col_2))},
		{COM_NONE            , INST_ENVTRANSITION , "Hero Light 1", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::hero_light_1))},
		{COM_NONE            , INST_ENVTRANSITION , "Hero Light 2", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::hero_light_2))},
		{COM_NONE            , INST_ENVTRANSITION , "Enable Fog", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::enable_fog))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Colour 1", vec3_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_col_1))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Colour 2", vec3_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_col_2))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Near Dist 1", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_near_dist_1))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Near Intensity 1", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_near_intensity_1))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Far Dist 1", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_far_dist_1))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Far Intensity 1", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_far_intensity_2))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Near Dist 2", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_near_dist_2))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Near Intensity 2", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_near_intensity_2))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Far Dist 2", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_far_dist_2))},
		{COM_NONE            , INST_ENVTRANSITION , "Fog Far Intensity 2", scalar_funcs(adapt_member_pointer(&EnvTransitionInstance::fog_far_intensity_2))},
		// Camera
		{COM_NONE            , INST_CAMERA        , "Class", scalar_funcs(adapt_reference_member_function<s32>(&CameraInstance::o_class))},
		// Sound
		{COM_NONE            , INST_SOUND         , "Class", scalar_funcs(adapt_reference_member_function<s32>(&SoundInstance::o_class))},
		{COM_NONE            , INST_SOUND         , "M Class", scalar_funcs(adapt_member_pointer(&SoundInstance::m_class))},
		{COM_NONE            , INST_SOUND         , "Range", scalar_funcs(adapt_member_pointer(&SoundInstance::range))},
		// GrindPath
		{COM_NONE            , INST_GRINDPATH     , "Wrap", scalar_funcs(adapt_member_pointer(&GrindPathInstance::wrap))},
		{COM_NONE            , INST_GRINDPATH     , "Inactive", scalar_funcs(adapt_member_pointer(&GrindPathInstance::inactive))},
		{COM_NONE            , INST_GRINDPATH     , "Unk 4", scalar_funcs(adapt_member_pointer(&GrindPathInstance::unknown_4))},
	};
	
	static const std::vector<InspectorField> rac1_fields = {
		{COM_NONE            , INST_MOBY      , "Unk 4    ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_4))},
		{COM_NONE            , INST_MOBY      , "Unk 8    ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_8))},
		{COM_NONE            , INST_MOBY      , "Unk c    ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_c))},
		{COM_NONE            , INST_MOBY      , "Unk 10   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_10))},
		{COM_NONE            , INST_MOBY      , "Unk 14   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_14))},
		{COM_NONE            , INST_MOBY      , "Unk 18   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_18))},
		{COM_NONE            , INST_MOBY      , "Unk 1c   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_1c))},
		{COM_NONE            , INST_MOBY      , "Unk 20   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_20))},
		{COM_NONE            , INST_MOBY      , "Unk 24   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_24))},
		{COM_NONE            , INST_MOBY      , "Unk 54   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_54))},
		{COM_NONE            , INST_MOBY      , "Unk 74   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac1_unknown_74))}
	};
	
	static const std::vector<InspectorField> rac23_fields = {
		{COM_NONE            , INST_MOBY      , "Unk 8    ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_8))},
		{COM_NONE            , INST_MOBY      , "Unk c    ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_c))},
		{COM_NONE            , INST_MOBY      , "Unk 18   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_18))},
		{COM_NONE            , INST_MOBY      , "Unk 1c   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_1c))},
		{COM_NONE            , INST_MOBY      , "Unk 20   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_20))},
		{COM_NONE            , INST_MOBY      , "Unk 24   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_24))},
		{COM_NONE            , INST_MOBY      , "Unk 4c   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_4c))},
		{COM_NONE            , INST_MOBY      , "Unk 84   ", scalar_funcs(adapt_member_pointer(&MobyInstance::rac23_unknown_84))}
	};
	
	static const std::vector<InspectorField> gc_uya_dl_fields = {
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Enable Fog Params", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::enable_fog_params))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Fog Near Intensity", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::fog_near_intensity))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Fog Far Intensity", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::fog_far_intensity))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Fog Colour", vec3_funcs(adapt_member_pointer(&EnvSamplePointInstance::fog_col))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Fog Near Dist", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::fog_near_dist))},
		{COM_NONE            , INST_ENVSAMPLEPOINT, "Fog Far Dist", scalar_funcs(adapt_member_pointer(&EnvSamplePointInstance::fog_far_dist))},
	};
	
	int invalidation_id = (int) get_invaliation_id(lvl.instances());
	
	ImGui::PushID(invalidation_id);
	if (ImGui::BeginTable("header", 2)) {
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);
		draw_fields(lvl, header_fields);
		ImGui::EndTable();
	}
	ImGui::PopID();
	
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4, 4));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
	
	transform_inspector(lvl);
	
	if (ImGui::CollapsingHeader("Attributes")) {
		ImGui::PushID(invalidation_id);
		if (ImGui::BeginTable("inspector", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
			ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);
			draw_fields(lvl, fields);
			if (lvl.game == Game::RAC) {
				draw_fields(lvl, rac1_fields);
			}
			if (lvl.game == Game::GC || lvl.game == Game::UYA) {
				draw_fields(lvl, rac23_fields);
			}
			if (lvl.game != Game::RAC) {
				draw_fields(lvl, gc_uya_dl_fields);
			}
			ImGui::EndTable();
		}
		ImGui::PopID();
	}
	
	ImGui::PopStyleVar();
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	
	const CppType* pvar_type = get_pvar_type_for_selection(lvl);
	if (pvar_type && ImGui::CollapsingHeader("Pvars")) {
		ImGui::PushID(invalidation_id);
		pvar_inspector(lvl, *pvar_type);
		ImGui::PopID();
	}
}

// This is needed so that when you switch objects imgui doesn't get confused
// and use state related to one object for a different object. This probably
// isn't perfect but should work in most cases.
static u32 get_invaliation_id(const Instances& instances)
{
	static u32 id = 0;
	static std::vector<bool> selected_back, selected_front;
	// Determine which instances are selected.
	selected_back.clear();
	instances.for_each([&](const Instance& inst) {
		selected_back.emplace_back(inst.selected);
	});
	if (selected_back != selected_front) {
		id++;
	}
	// Swap the buffers.
	std::vector<bool> temp = std::move(selected_back);
	selected_back = std::move(selected_front);
	selected_front = std::move(temp);
	return id;
}

static void draw_fields(Level& lvl, const std::vector<InspectorField>& fields)
{
	for (const InspectorField& field : fields) {
		verify_fatal(field.funcs.lane_count <= MAX_LANES);
		if (should_draw_field(lvl, field)) {
			// If selected objects have fields with conflicting values, we
			// shouldn't draw the old value.
			bool values_equal[MAX_LANES];
			should_draw_current_values(values_equal, lvl, field);
			
			Instance* first = nullptr;
			lvl.instances().for_each([&](Instance& inst) {
				if (first == nullptr && inst.selected) {
					first = &inst;
				}
			});
			verify_fatal(first != nullptr);
			
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", field.name);
			
			ImGui::TableNextColumn();
			ImGui::PushID((int) field.required_type);
			ImGui::PushID(field.name);
			field.funcs.draw(lvl, *first, values_equal);
			ImGui::PopID();
			ImGui::PopID();
		}
	}
}

static InspectorFieldFuncs type_funcs()
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 1;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		return lhs.type() == rhs.type();
	};
	funcs.draw = [](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		if (values_equal[0]) {
			const char* type = instance_type_to_string(first.type());
			ImGui::Text("%s", type);
		} else {
			ImGui::Text("<multiple selected>");
		}
	};
	return funcs;
}

static InspectorFieldFuncs id_funcs()
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 1;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		return lhs.id() == rhs.id();
	};
	funcs.draw = [](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		if (values_equal[0]) {
			ImGui::Text("%d", first.id().value);
		} else {
			ImGui::Text("<multiple selected>");
		}
	};
	return funcs;
}

template <typename Value>
static InspectorFieldFuncs scalar_funcs(InspectorGetterSetter<Value> getset)
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 1;
	funcs.compare = [getset](Instance& lhs, Instance& rhs, s32) {
		return getset.get(lhs) == getset.get(rhs);
	};
	funcs.draw = [getset](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		Value value = getset.get(first);
		std::string value_str;
		if (values_equal[0]) {
			value_str = std::to_string(value);
		}
		ImGui::PushItemWidth(calc_remaining_item_width());
		bool changed = ImGui::InputText("", &value_str);
		ImGui::PopItemWidth();
		if (changed) {
			Opt<Value> new_value = string_to_scalar<Value>(value_str);
			if (new_value.has_value()) {
				std::array<bool, MAX_LANES> dummy;
				apply_to_all_selected<0>(lvl, *new_value, dummy, getset);
			}
		}
	};
	return funcs;
}

static InspectorFieldFuncs bool_funcs(InspectorGetterSetter<bool> getset)
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 1;
	funcs.compare = [getset](Instance& lhs, Instance& rhs, s32) {
		return getset.get(lhs) == getset.get(rhs);
	};
	funcs.draw = [getset](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		bool value = getset.get(first);
		if (ImGui::Checkbox("##checkbox", &value)) {
			std::array<bool, MAX_LANES> dummy;
			apply_to_all_selected<0>(lvl, value, dummy, getset);
		}
	};
	return funcs;
}

static InspectorFieldFuncs vec3_funcs(InspectorGetterSetter<glm::vec3> getset)
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 3;
	funcs.compare = [getset](Instance& lhs, Instance& rhs, s32 lane) {
		return getset.get(lhs)[lane] == getset.get(rhs)[lane];
	};
	funcs.draw = [getset](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		glm::vec3 value = getset.get(first);
		std::array<std::string, MAX_LANES> strings = vec4_to_strings(glm::vec4(value, -1.f), values_equal);
		std::array<bool, MAX_LANES> changed;
		if (inspector_input_text_n(strings, changed, 3)) {
			Opt<glm::vec4> new_value_4 = strings_to_vec4(strings, changed);
			if (new_value_4.has_value()) {
				glm::vec3 new_value = glm::vec3(*new_value_4);
				apply_to_all_selected<3>(lvl, new_value, changed, getset);
			}
		}
	};
	return funcs;
}

static InspectorFieldFuncs vec4_funcs(InspectorGetterSetter<glm::vec4> getset)
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 4;
	funcs.compare = [getset](Instance& lhs, Instance& rhs, s32 lane) {
		return getset.get(lhs)[lane] == getset.get(rhs)[lane];
	};
	funcs.draw = [getset](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		glm::vec4 value = getset.get(first);
		std::array<std::string, 4> strings = vec4_to_strings(value, values_equal);
		std::array<bool, MAX_LANES> changed;
		if (inspector_input_text_n(strings, changed, 4)) {
			Opt<glm::vec4> new_value = strings_to_vec4(strings, changed);
			if (new_value.has_value()) {
				apply_to_all_selected<4>(lvl, *new_value, changed, getset);
			}
		}
	};
	return funcs;
}

template <typename ThisInstance>
static InspectorFieldFuncs foreign_id_funcs(InstanceType foreign_type, s32 ThisInstance::*field)
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 1;
	funcs.compare = [field](Instance& lhs, Instance& rhs, s32) {
		auto& this_lhs = dynamic_cast<ThisInstance&>(lhs);
		auto& this_rhs = dynamic_cast<ThisInstance&>(rhs);
		return (this_lhs.*field) == (this_rhs.*field);
	};
	funcs.draw = [foreign_type, field](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		auto& this_first = dynamic_cast<ThisInstance&>(first);
		s32 value = (this_first.*field);
		std::string value_str;
		if (values_equal[0]) {
			value_str = std::to_string(value);
		}
		bool changed = false;
		ImGui::PushItemWidth(calc_remaining_item_width());
		if (ImGui::BeginCombo("##combo", value_str.c_str())) {
			lvl.instances().for_each([&](Instance& inst) {
				if (inst.type() == foreign_type) {
					s32 new_value = inst.id().value;
					std::string new_value_str = std::to_string(new_value);
					if (ImGui::Selectable(new_value_str.c_str())) {
						value = new_value;
						changed = true;
					}
				}
			});
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
		if (changed) {
			std::array<bool, MAX_LANES> dummy;
			apply_to_all_selected<0>(lvl, value, dummy, adapt_member_pointer(field));
		}
	};
	return funcs;
}

static InspectorFieldFuncs camera_collision_funcs()
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 1;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		return lhs.camera_collision() == rhs.camera_collision();
	};
	funcs.draw = [](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		CameraCollisionParams first_params = first.camera_collision();
		
		bool changed = false;
		ImGui::PopStyleColor();
		changed |= ImGui::Checkbox("##cam_coll_enabled", &first_params.enabled);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		ImGui::SameLine();
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
		if (!first_params.enabled) {
			flags |= ImGuiInputTextFlags_ReadOnly;
		}
		f32 remaining_width = calc_remaining_item_width();
		ImGui::PushItemWidth(remaining_width / 3);
		changed |= ImGui::InputInt("##flags", &first_params.flags, 1, 100, flags);
		ImGui::SameLine();
		changed |= ImGui::InputInt("##i_value", &first_params.i_value, 1, 100, flags);
		ImGui::SameLine();
		changed |= ImGui::InputFloat("##f_value", &first_params.f_value, 0.f, 0.f, "%f", flags);
		ImGui::PopItemWidth();
		
		if (changed) {
			std::array<bool, MAX_LANES> dummy;
			apply_to_all_selected<0>(lvl, first_params, dummy, {
				[](Instance& inst) {
					return inst.camera_collision();
				},
				[](Instance& inst, CameraCollisionParams value) {
					inst.camera_collision() = value;
				}
			});
		}
	};
	return funcs;
}

static InspectorFieldFuncs moby_rooted_funcs()
{
	InspectorFieldFuncs funcs;
	funcs.lane_count = 1;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		auto& moby_lhs = dynamic_cast<MobyInstance&>(lhs);
		auto& moby_rhs = dynamic_cast<MobyInstance&>(rhs);
		return moby_lhs.is_rooted == moby_rhs.is_rooted && moby_lhs.rooted_distance == moby_rhs.rooted_distance;
	};
	funcs.draw = [](Level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		MobyInstance& moby_first = dynamic_cast<MobyInstance&>(first);
		bool is_rooted = moby_first.is_rooted;
		f32 rooted_distance = moby_first.rooted_distance;
		
		bool changed = false;
		ImGui::PopStyleColor();
		changed |= ImGui::Checkbox("##is_rooted", &is_rooted);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
		ImGui::SameLine();
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_None;
		if (!is_rooted) {
			flags |= ImGuiInputTextFlags_ReadOnly;
		}
		ImGui::PushItemWidth(calc_remaining_item_width());
		changed |= ImGui::InputFloat("##rooted_distance", &rooted_distance, 0.f, 0.f, "%f", flags);
		ImGui::PopItemWidth();
		
		if (changed) {
			std::array<bool, MAX_LANES> dummy;
			apply_to_all_selected<0>(lvl, std::pair<bool, f32>(is_rooted, rooted_distance), dummy, {
				[](Instance& inst) {
					auto& moby = dynamic_cast<MobyInstance&>(inst);
					return std::pair<bool, f32>(moby.is_rooted, moby.rooted_distance);
				},
				[](Instance& inst, std::pair<bool, f32> value) {
					auto& moby = dynamic_cast<MobyInstance&>(inst);
					moby.is_rooted = value.first;
					moby.rooted_distance = value.second;
				}
			});
		}
	};
	return funcs;
}

static bool should_draw_field(Level& lvl, const InspectorField& field)
{
	bool one_instance_has_field = false;
	bool all_instances_have_field = true;
	lvl.instances().for_each([&](Instance& inst) {
		if (inst.selected) {
			bool required_type = field.required_type == INST_NONE || inst.type() == field.required_type;
			if (inst.has_component(field.required_component) && required_type) {
				one_instance_has_field = true;
			} else {
				all_instances_have_field = false;
			}
		}
	});
	return one_instance_has_field && all_instances_have_field;
}

static void should_draw_current_values(
	bool values_equal[MAX_LANES], Level& lvl, const InspectorField& field)
{
	for (s32 lane = 0; lane < MAX_LANES; lane++) {
		values_equal[lane] = true;
	}
	Instance* last_inst = nullptr;
	lvl.instances().for_each([&](Instance& inst) {
		if (inst.selected) {
			if (last_inst != nullptr) {
				for (s32 lane = 0; lane < field.funcs.lane_count; lane++) {
					if (!field.funcs.compare(*last_inst, inst, lane)) {
						values_equal[lane] = false;
					}
				}
			}
			last_inst = &inst;
		}
	});
}

template <s32 lane_count, typename Value>
static void apply_to_all_selected(
	Level& lvl, Value value, std::array<bool, MAX_LANES> lanes, InspectorGetterSetter<Value> funcs)
{
	struct InspectorCommand
	{
		InspectorGetterSetter<Value> funcs;
		std::array<bool, MAX_LANES> lanes;
		std::vector<InstanceId> ids;
		Value value;
		std::vector<Value> old_values;
	};
	
	InspectorCommand data;
	data.funcs = funcs;
	data.lanes = lanes;
	data.ids = lvl.instances().selected_instances();
	data.value = value;
	
	lvl.instances().for_each([&](Instance& inst) {
		if (contains(data.ids, inst.id())) {
			data.old_values.push_back(funcs.get(inst));
		}
	});
	
	lvl.push_command<InspectorCommand>(std::move(data),
		[](Level& lvl, InspectorCommand& data) {
			lvl.instances().for_each([&](Instance& inst) {
				if (contains(data.ids, inst.id())) {
					if constexpr(lane_count > 0) {
						Value temp = data.funcs.get(inst);
						for (s32 lane = 0; lane < lane_count; lane++) {
							if (data.lanes[lane]) {
								temp[lane] = data.value[lane];
							}
						}
						data.funcs.set(inst, temp);
					} else {
						data.funcs.set(inst, data.value);
					}
				}
			});
		},
		[](Level& lvl, InspectorCommand& data) {
			size_t i = 0;
			lvl.instances().for_each([&](Instance& inst) {
				if (contains(data.ids, inst.id())) {
					data.funcs.set(inst, data.old_values[i++]);
				}
			});
		}
	);
}

template <typename Value>
static InspectorGetterSetter<Value> adapt_getter_setter(
	Value (Instance::*getter)() const, void (Instance::*setter)(Value))
{
	InspectorGetterSetter<Value> funcs;
	funcs.get = [getter](Instance& inst) {
		return (inst.*getter)();
	};
	funcs.set = [setter](Instance& inst, Value value) {
		(inst.*setter)(value);
	};
	return funcs;
}

template <typename Value>
static InspectorGetterSetter<Value> adapt_reference_member_function(Value& (Instance::*member_function)())
{
	InspectorGetterSetter<Value> funcs;
	funcs.get = [member_function](Instance& inst) {
		Value value = (inst.*member_function)();
		return value;
	};
	funcs.set = [member_function](Instance& inst, Value value) {
		(inst.*member_function)() = value;
	};
	return funcs;
}

template <typename Value, typename ThisInstance>
static InspectorGetterSetter<Value> adapt_member_pointer(Value ThisInstance::*member_pointer)
{
	InspectorGetterSetter<Value> funcs;
	funcs.get = [member_pointer](Instance& inst) {
		return dynamic_cast<ThisInstance&>(inst).*member_pointer;
	};
	funcs.set = [member_pointer](Instance& inst, Value value) {
		dynamic_cast<ThisInstance&>(inst).*member_pointer = value;
	};
	return funcs;
}

static float calc_remaining_item_width()
{
	return ImGui::GetWindowSize().x - ImGui::GetCursorPos().x - 16.f;
}

bool inspector_input_text_n(
	std::array<std::string, MAX_LANES>& strings, std::array<bool, MAX_LANES>& changed, int lane_count)
{
	for (s32 lane = 0; lane < MAX_LANES; lane++) {
		changed[lane] = false;
	}
	bool any_lane_changed = false;
	ImGui::PushMultiItemsWidths(lane_count, calc_remaining_item_width());
	for (s32 lane = 0; lane < lane_count; lane++) {
		ImGui::PushID(lane);
		if (lane > 0) {
			ImGui::SameLine();
		}
		changed[lane] = ImGui::InputText("", &strings[lane], ImGuiInputTextFlags_EnterReturnsTrue);
		any_lane_changed |= changed[lane];
		ImGui::PopID(); // lane
		ImGui::PopItemWidth();
	}
	return any_lane_changed;
}

static std::array<std::string, MAX_LANES> vec4_to_strings(glm::vec4 vec, bool values_equal[MAX_LANES])
{
	std::array<std::string, MAX_LANES> strings;
	for (s32 lane = 0; lane < 4; lane++) {
		if (values_equal[lane]) {
			strings[lane] = std::to_string(vec[lane]);
		}
	}
	return strings;
}

static Opt<glm::vec4> strings_to_vec4(
	std::array<std::string, MAX_LANES>& strings, std::array<bool, MAX_LANES>& changed)
{
	glm::vec4 vec;
	try {
		for (s32 lane = 0; lane < 4; lane++) {
			if (changed[lane]) {
				vec[lane] = std::stof(strings[lane]);
			} else {
				vec[lane] = -1.f; // Don't care.
			}
		}
	} catch (std::logic_error&) {
		return {};
	}
	return vec;
}

template <typename Scalar>
static Opt<Scalar> string_to_scalar(std::string& string)
{
	try {
		if constexpr(std::is_floating_point_v<Scalar>) {
			return std::stof(string);
		} else {
			return std::stoi(string);
		}
	} catch (std::logic_error&) {
		return {};
	}
}

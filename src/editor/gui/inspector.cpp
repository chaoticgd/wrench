/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "../app.h"

const char* Inspector::title_text() const {
	return "Inspector";
};

ImVec2 Inspector::initial_size() const {
	return ImVec2(200, 800);
}

static const s32 MAX_LANES = 4;
struct InspectorFieldFuncs {
	std::function<bool(Instance& lhs, Instance& rhs, s32 lane)> compare;
	std::function<void(level& lvl, Instance& first, bool values_equal[MAX_LANES])> draw;
};

struct InspectorField {
	InstanceComponent required_component;
	InstanceType required_type;
	s32 lane_count;
	const char* name;
	InspectorFieldFuncs funcs;
};

static InspectorFieldFuncs position_funcs();
static InspectorFieldFuncs rotation_funcs();
static InspectorFieldFuncs pvar_funcs();
template <typename Value>
static InspectorFieldFuncs member_func_pointer_funcs(Value& (Instance::*member_function)());
template <typename Value, typename ThisInstance>
static InspectorFieldFuncs member_pointer_funcs(Value ThisInstance::*field);
template <typename ThisInstance>
static InspectorFieldFuncs foreign_id_funcs(InstanceType foreign_type, s32 ThisInstance::*field);
static InspectorFieldFuncs moby_rooted_funcs();

static bool should_draw_field(level& lvl, InspectorField& field);
static void should_draw_current_values(bool values_equal[MAX_LANES], level& lvl, InspectorField& field);

template <typename Value>
struct InspectorGetterSetterFuncs {
	std::function<Value(Instance& inst)> get;
	std::function<void(Instance& inst, std::array<bool, MAX_LANES> lanes, Value value)> set;
};
template <typename Value>
static void apply_to_all_selected(level& lvl, Value value, std::array<bool, MAX_LANES> lanes, InspectorGetterSetterFuncs<Value> funcs);

static float calc_remaining_item_width();
static bool inspector_input_text_n(std::array<std::string, 3>& strings, std::array<bool, MAX_LANES>& changed, int lane_count);
static std::array<std::string, 3> vec3_to_strings(glm::vec3 vec, bool values_equal[MAX_LANES]);
static Opt<glm::vec3> strings_to_vec3(std::array<std::string, 3>& strings);
template <typename Scalar>
static Opt<Scalar> string_to_scalar(std::string& string);

void Inspector::render(app& a) {
	if(!a.get_level()) {
		ImGui::Text("<no level>");
		return;
	}
	level& lvl = *a.get_level();
	
	std::vector<InspectorField> fields = {
		{COM_TRANSFORM    , INST_NONE, 3, "Position ", position_funcs()},
		{COM_TRANSFORM    , INST_NONE, 3, "Rotation ", rotation_funcs()},
		{COM_PVARS        , INST_NONE, 1, "Pvars    ", pvar_funcs()},
		{COM_DRAW_DISTANCE, INST_NONE, 1, "Draw Dist", member_func_pointer_funcs(&Instance::draw_distance)},
		{COM_NONE         , INST_MOBY, 1, "Mission  ", member_pointer_funcs(&MobyInstance::mission)},
		{COM_NONE         , INST_MOBY, 1, "UID      ", member_pointer_funcs(&MobyInstance::uid)},
		{COM_NONE         , INST_MOBY, 1, "Bolts    ", member_pointer_funcs(&MobyInstance::bolts)},
		{COM_NONE         , INST_MOBY, 1, "Class    ", member_pointer_funcs(&MobyInstance::o_class)},
		{COM_NONE         , INST_MOBY, 1, "Updt Dist", member_pointer_funcs(&MobyInstance::update_distance)},
		{COM_NONE         , INST_MOBY, 1, "Group    ", member_pointer_funcs(&MobyInstance::group)},
		{COM_NONE         , INST_MOBY, 1, "Rooted   ", moby_rooted_funcs()},
		{COM_NONE         , INST_MOBY, 1, "Occlusion", member_pointer_funcs(&MobyInstance::occlusion)},
		{COM_NONE         , INST_MOBY, 1, "Mode Bits", member_pointer_funcs(&MobyInstance::mode_bits)},
		{COM_NONE         , INST_MOBY, 1, "Light    ", foreign_id_funcs(INST_LIGHT, &MobyInstance::light)}
	};
	
	for(InspectorField& field : fields) {
		assert(field.lane_count <= MAX_LANES);
		if(should_draw_field(lvl, field)) {
			// If selected objects have fields with conflicting values, we
			// shouldn't draw the old value.
			bool values_equal[MAX_LANES];
			should_draw_current_values(values_equal, lvl, field);
			
			Instance* first = nullptr;
			lvl.gameplay().for_each_instance([&](Instance& inst) {
				if(first == nullptr && inst.selected) {
					first = &inst;
				}
			});
			
			ImGui::AlignTextToFramePadding();
			ImGui::Text("%s", field.name);
			ImGui::SameLine();
			assert(first != nullptr);
			ImGui::PushID(field.name);
			field.funcs.draw(lvl, *first, values_equal);
			ImGui::PopID();
		}
	}
}

static InspectorFieldFuncs position_funcs() {
	InspectorFieldFuncs funcs;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		return lhs.position()[lane] == rhs.position()[lane];
	};
	funcs.draw = [](level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		glm::vec3 value = first.position();
		std::array<std::string, 3> strings = vec3_to_strings(value, values_equal);
		std::array<bool, MAX_LANES> changed;
		if(inspector_input_text_n(strings, changed, 3)) {
			Opt<glm::vec3> new_value = strings_to_vec3(strings);
			if(!new_value.has_value()) {
				return;
			}
			apply_to_all_selected<glm::vec3>(lvl, *new_value, changed, {
				[](Instance& inst) { return inst.position(); },
				[](Instance& inst, std::array<bool, MAX_LANES> lanes, glm::vec3 value) {
					auto temp = inst.position();
					for(s32 lane = 0; lane < MAX_LANES; lane++) {
						if(lanes[lane]) {
							temp[lane] = value[lane];
						}
					}
					inst.set_position(temp);
				}
			});
		}
	};
	return funcs;
}

static InspectorFieldFuncs rotation_funcs() {
	InspectorFieldFuncs funcs;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		return lhs.rotation()[lane] == rhs.rotation()[lane];
	};
	funcs.draw = [](level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		glm::vec3 value = first.rotation();
		std::array<std::string, 3> strings = vec3_to_strings(value, values_equal);
		std::array<bool, MAX_LANES> changed;
		if(inspector_input_text_n(strings, changed, 3)) {
			Opt<glm::vec3> new_value = strings_to_vec3(strings);
			if(!new_value.has_value()) {
				return;
			}
			apply_to_all_selected<glm::vec3>(lvl, *new_value, changed, {
				[](Instance& inst) { return inst.rotation(); },
				[](Instance& inst, std::array<bool, MAX_LANES> lanes, glm::vec3 value) {
					auto temp = inst.rotation();
					for(s32 lane = 0; lane < MAX_LANES; lane++) {
						if(lanes[lane]) {
							temp[lane] = value[lane];
						}
					}
					inst.set_rotation(temp);
				}
			});
		}
	};
	return funcs;
}

static InspectorFieldFuncs pvar_funcs() {
	InspectorFieldFuncs funcs;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		return lhs.pvars().size() == 0 && rhs.pvars().size() == 0;
	};
	funcs.draw = [](level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		if(values_equal[0]) {
			ImGui::Text("<empty>");
		} else {
			ImGui::Button("View");
		}
	};
	return funcs;
}

template <typename Value>
static InspectorFieldFuncs member_func_pointer_funcs(Value& (Instance::*member_function)()) {
	InspectorFieldFuncs funcs;
	funcs.compare = [member_function](Instance& lhs, Instance& rhs, s32) {
		return (lhs.*member_function)() == (rhs.*member_function)();
	};
	funcs.draw = [member_function](level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		Value value = (first.*member_function)();
		std::string value_str;
		if(values_equal[0]) {
			value_str = std::to_string(value);
		}
		ImGui::PushItemWidth(calc_remaining_item_width());
		bool changed = ImGui::InputText("", &value_str);
		ImGui::PopItemWidth();
		if(changed) {
			Opt<Value> new_value = string_to_scalar<Value>(value_str);
			if(new_value.has_value()) {
				std::array<bool, MAX_LANES> dummy;
				apply_to_all_selected<Value>(lvl, *new_value, dummy, {
					[member_function](Instance& inst) {
						Value value = (inst.*member_function)();
						return value;
					},
					[member_function](Instance& inst, std::array<bool, MAX_LANES>, Value value) {
						(inst.*member_function)() = value;
					}
				});
			}
		}
	};
	return funcs;
}

template <typename Value, typename ThisInstance>
static InspectorFieldFuncs member_pointer_funcs(Value ThisInstance::*field) {
	InspectorFieldFuncs funcs;
	funcs.compare = [field](Instance& lhs, Instance& rhs, s32) {
		auto& this_lhs = dynamic_cast<ThisInstance&>(lhs);
		auto& this_rhs = dynamic_cast<ThisInstance&>(rhs);
		return (this_lhs.*field) == (this_rhs.*field);
	};
	funcs.draw = [field](level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		auto& this_first = dynamic_cast<ThisInstance&>(first);
		Value value = (this_first.*field);
		std::string value_str;
		if(values_equal[0]) {
			value_str = std::to_string(value);
		}
		ImGui::PushItemWidth(calc_remaining_item_width());
		bool changed = ImGui::InputText("", &value_str);
		ImGui::PopItemWidth();
		if(changed) {
			Opt<Value> new_value = string_to_scalar<Value>(value_str);
			if(new_value.has_value()) {
				std::array<bool, MAX_LANES> dummy;
				apply_to_all_selected<Value>(lvl, *new_value, dummy, {
					[field](Instance& inst) {
						auto& this_inst = dynamic_cast<ThisInstance&>(inst);
						return this_inst.*field;
					},
					[field](Instance& inst, std::array<bool, MAX_LANES>, Value value) {
						auto& this_inst = dynamic_cast<ThisInstance&>(inst);
						this_inst.*field = value;
					}
				});
			}
		}
	};
	return funcs;
}

template <typename ThisInstance>
static InspectorFieldFuncs foreign_id_funcs(InstanceType foreign_type, s32 ThisInstance::*field) {
	InspectorFieldFuncs funcs;
	funcs.compare = [field](Instance& lhs, Instance& rhs, s32) {
		auto& this_lhs = dynamic_cast<ThisInstance&>(lhs);
		auto& this_rhs = dynamic_cast<ThisInstance&>(rhs);
		return (this_lhs.*field) == (this_rhs.*field);
	};
	funcs.draw = [foreign_type, field](level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		auto& this_first = dynamic_cast<ThisInstance&>(first);
		s32 value = (this_first.*field);
		std::string value_str;
		if(values_equal[0]) {
			value_str = std::to_string(value);
		}
		bool changed = false;
		ImGui::PushItemWidth(calc_remaining_item_width());
		if(ImGui::BeginCombo("##combo", value_str.c_str())) {
			lvl.gameplay().for_each_instance([&](Instance& inst) {
				if(inst.type() == foreign_type) {
					s32 new_value = inst.id().value;
					std::string new_value_str = std::to_string(new_value);
					if(ImGui::Selectable(new_value_str.c_str())) {
						value = new_value;
						changed = true;
					}
				}
			});
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
		if(changed) {
			std::array<bool, MAX_LANES> dummy;
			apply_to_all_selected<s32>(lvl, value, dummy, {
				[field](Instance& inst) {
					auto& this_inst = dynamic_cast<ThisInstance&>(inst);
					return this_inst.*field;
				},
				[field](Instance& inst, std::array<bool, MAX_LANES>, s32 value) {
					auto& this_inst = dynamic_cast<ThisInstance&>(inst);
					this_inst.*field = value;
				}
			});
		}
	};
	return funcs;
}

static InspectorFieldFuncs moby_rooted_funcs() {
	InspectorFieldFuncs funcs;
	funcs.compare = [](Instance& lhs, Instance& rhs, s32 lane) {
		auto& moby_lhs = dynamic_cast<MobyInstance&>(lhs);
		auto& moby_rhs = dynamic_cast<MobyInstance&>(rhs);
		return moby_lhs.is_rooted == moby_rhs.is_rooted && moby_lhs.rooted_distance == moby_rhs.rooted_distance;
	};
	funcs.draw = [](level& lvl, Instance& first, bool values_equal[MAX_LANES]) {
		MobyInstance& moby_first = dynamic_cast<MobyInstance&>(first);
		bool is_rooted = moby_first.is_rooted;
		f32 rooted_distance = moby_first.rooted_distance;
		
		bool changed = false;
		changed |= ImGui::Checkbox("##is_rooted", &is_rooted);
		ImGui::SameLine();
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
		if(!is_rooted) {
			flags |= ImGuiInputTextFlags_ReadOnly;
		}
		ImGui::PushItemWidth(calc_remaining_item_width());
		changed |= ImGui::InputFloat("##rooted_distance", &rooted_distance, 0.f, 0.f, "%f", flags);
		ImGui::PopItemWidth();
		
		if(changed) {
			std::array<bool, MAX_LANES> dummy;
			apply_to_all_selected<std::pair<bool, f32>>(lvl, {is_rooted, rooted_distance}, dummy, {
				[](Instance& inst) {
					auto& moby = dynamic_cast<MobyInstance&>(inst);
					return std::pair<bool, f32>(moby.is_rooted, moby.rooted_distance);
				},
				[](Instance& inst, std::array<bool, MAX_LANES>, std::pair<bool, f32> value) {
					auto& moby = dynamic_cast<MobyInstance&>(inst);
					moby.is_rooted = value.first;
					moby.rooted_distance = value.second;
				}
			});
		}
	};
	return funcs;
}

static bool should_draw_field(level& lvl, InspectorField& field) {
	bool one_instance_has_field = false;
	bool all_instances_have_field = true;
	lvl.gameplay().for_each_instance([&](Instance& inst) {
		if(inst.selected) {
			bool required_type = field.required_type == INST_NONE || inst.type() == field.required_type;
			if(inst.has_component(field.required_component) && required_type) {
				one_instance_has_field = true;
			} else {
				all_instances_have_field = false;
			}
		}
	});
	return one_instance_has_field && all_instances_have_field;
}

static void should_draw_current_values(bool values_equal[MAX_LANES], level& lvl, InspectorField& field) {
	for(s32 lane = 0; lane < MAX_LANES; lane++) {
		values_equal[lane] = true;
	}
	Instance* last_inst = nullptr;
	lvl.gameplay().for_each_instance([&](Instance& inst) {
		if(inst.selected) {
			if(last_inst != nullptr) {
				for(s32 lane = 0; lane < field.lane_count; lane++) {
					if(!field.funcs.compare(*last_inst, inst, lane)) {
						values_equal[lane] = false;
					}
				}
			}
			last_inst = &inst;
		}
	});
}

template <typename Value>
static void apply_to_all_selected(level& lvl, Value value, std::array<bool, MAX_LANES> lanes, InspectorGetterSetterFuncs<Value> funcs) {
	std::vector<InstanceId> ids = lvl.gameplay().selected_instances();
	
	std::vector<Value> old_values;
	lvl.gameplay().for_each_instance([&](Instance& inst) {
		if(contains(ids, inst.id())) {
			old_values.push_back(funcs.get(inst));
		}
	});
	
	lvl.push_command(
		[funcs, lanes, ids, value](level& lvl) {
			lvl.gameplay().for_each_instance([&](Instance& inst) {
				if(contains(ids, inst.id())) {
					funcs.set(inst, lanes, value);
				}
			});
		},
		[funcs, lanes, ids, old_values](level& lvl) {
			size_t i = 0;
			lvl.gameplay().for_each_instance([&](Instance& inst) {
				if(contains(ids, inst.id())) {
					funcs.set(inst, lanes, old_values[i++]);
				}
			});
		}
	);
}

static float calc_remaining_item_width() {
	return ImGui::GetWindowSize().x - ImGui::GetCursorPos().x - 16.f;
}

static bool inspector_input_text_n(std::array<std::string, 3>& strings, std::array<bool, MAX_LANES>& changed, int lane_count) {
	for(s32 lane = 0; lane < MAX_LANES; lane++) {
		changed[lane] = false;
	}
	bool any_lane_changed = false;
	ImGui::PushMultiItemsWidths(lane_count, calc_remaining_item_width());
	for(s32 lane = 0; lane < lane_count; lane++) {
		ImGui::PushID(lane);
		if(lane > 0) {
			ImGui::SameLine();
		}
		changed[lane] = ImGui::InputText("", &strings[lane], ImGuiInputTextFlags_EnterReturnsTrue);
		any_lane_changed |= changed[lane];
		ImGui::PopID(); // lane
		ImGui::PopItemWidth();
	}
	return any_lane_changed;
}

static std::array<std::string, 3> vec3_to_strings(glm::vec3 vec, bool values_equal[MAX_LANES]) {
	std::array<std::string, 3> strings;
	for(s32 lane = 0; lane < 3; lane++) {
		if(values_equal[lane]) {
			strings[lane] = std::to_string(vec[lane]);
		}
	}
	return strings;
}

static Opt<glm::vec3> strings_to_vec3(std::array<std::string, 3>& strings) {
	glm::vec3 vec;
	try {
		for(s32 lane = 0; lane < 3; lane++) {
			vec[lane] = std::stof(strings[lane]);
		}
	} catch(std::logic_error&) {
		return {};
	}
	return vec;
}

template <typename Scalar>
static Opt<Scalar> string_to_scalar(std::string& string) {
	try {
		if constexpr(std::is_floating_point_v<Scalar>) {
			return std::stof(string);
		} else {
			return std::stoi(string);
		}
	} catch(std::logic_error&) {
		return {};
	}
}

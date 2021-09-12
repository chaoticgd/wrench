
#include "collada.h"

DaeScene mesh_to_dae(Mesh mesh) {
	DaeNode node;
	node.name = "node";
	node.mesh = 0;
	
	DaeScene scene;
	scene.nodes.push_back(node);
	scene.meshes.push_back(mesh);
	return scene;
}

DaeScene import_dae(std::vector<u8> src) {
	return {};
}

static void write_asset_metadata(OutBuffer dest);
static void write_library_effects(OutBuffer dest);
static void write_library_materials(OutBuffer dest);
static void write_library_geometries(OutBuffer dest, const std::vector<Mesh>& meshes);
template <typename Value>
static void write_float_array(OutBuffer dest, const std::vector<Value> src, s32 mesh, const char* name);
static void write_library_visual_scenes(OutBuffer dest, const DaeScene& scene);
static void write_node(OutBuffer dest, const DaeNode& node);

std::vector<u8> write_dae(const DaeScene& scene) {
	std::vector<u8> vec;
	OutBuffer dest(vec);
	dest.writelf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>");
	dest.writelf("<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">");
	write_asset_metadata(dest);
	write_library_effects(dest);
	write_library_materials(dest);
	write_library_geometries(dest, scene.meshes);
	write_library_visual_scenes(dest, scene);
	dest.writelf("</COLLADA>");
	return vec;
}

static void write_asset_metadata(OutBuffer dest) {
	dest.writelf("\t<asset>");
	dest.writelf("\t\t<contributor>");
	dest.writelf("\t\t\t<authoring_tool>Wrench WAD Utility</authoring_tool>");
	dest.writelf("\t\t</contributor>");
	dest.writelf("\t\t<created>%04d-%02d-%02dT%02d:%02d:%02d</created>", 1, 1, 1, 0, 0, 0);
	dest.writelf("\t\t<modified>%04d-%02d-%02dT%02d:%02d:%02d</modified>", 1, 1, 1, 0, 0, 0);
	dest.writelf("\t\t<unit name=\"meter\" meter=\"1\" />");
	dest.writelf("\t\t<up_axis>Z_UP</up_axis>");
	dest.writelf("\t</asset>");
}

static void write_library_effects(OutBuffer dest) {
	dest.writelf("\t<library_effects>");
	dest.writelf("\t\t<effect id=\"effect\">");
	dest.writelf("\t\t\t<profile_COMMON>");
	dest.writelf("\t\t\t\t<technique sid=\"standard\">");
	dest.writelf("\t\t\t\t\t<phong>");
	dest.writelf("\t\t\t\t\t</phong>");
	dest.writelf("\t\t\t\t</technique>");
	dest.writelf("\t\t\t</profile_COMMON>");
	dest.writelf("\t\t</effect>");
	dest.writelf("\t</library_effects>");
}

static void write_library_materials(OutBuffer dest) {
	dest.writelf("\t<library_materials>");
	dest.writelf("\t\t<material id=\"material\">");
	dest.writelf("\t\t\t<instance_effect url=\"#effect\"/>");
	dest.writelf("\t\t</material>");
	dest.writelf("\t</library_materials>");
}

static void write_library_geometries(OutBuffer dest, const std::vector<Mesh>& meshes) {
	dest.writelf("\t<library_geometries>");
	for(size_t i = 0; i < meshes.size(); i++) {
		const Mesh& mesh = meshes[i];
		dest.writelf("\t\t<geometry id=\"mesh_%d\">", i);
		dest.writelf("\t\t\t<mesh>");
		dest.writelf("\t\t\t\t<source id=\"mesh_%d_positions\">", i);
		write_float_array(dest, mesh.positions, i, "positions");
		dest.writelf("\t\t\t\t\t<technique_common>");
		dest.writelf("\t\t\t\t\t\t<accessor count=\"%d\" offset=\"0\" source=\"mesh_%d_positions_array\" stride=\"3\">", mesh.positions.size(), i);
		dest.writelf("\t\t\t\t\t\t\t<param name=\"X\" type=\"float\" />");
		dest.writelf("\t\t\t\t\t\t\t<param name=\"Y\" type=\"float\" />");
		dest.writelf("\t\t\t\t\t\t\t<param name=\"Z\" type=\"float\" />");
		dest.writelf("\t\t\t\t\t\t</accessor>");
		dest.writelf("\t\t\t\t\t</technique_common>");
		dest.writelf("\t\t\t\t</source>");
		if(mesh.texture_coords.has_value()) {
			dest.writelf("\t\t\t\t<source id=\"mesh_%d_texture_coords\">", i);
			write_float_array(dest, *mesh.texture_coords, i, "texture_coords");
			dest.writelf("\t\t\t\t</source>");
		}
		dest.writelf("\t\t\t\t<vertices id=\"mesh_%d_vertices\">", i);
		dest.writelf("\t\t\t\t\t<input semantic=\"POSITION\" source=\"#mesh_%d_positions\" />", i);
		dest.writelf("\t\t\t\t</vertices>");
		dest.writelf("\t\t\t\t<polylist count=\"%d\" material=\"defaultMaterial\">", mesh.tris.size() + mesh.quads.size());
		dest.writelf("\t\t\t\t\t<input offset=\"0\" semantic=\"VERTEX\" source=\"#mesh_%d_vertices\" />", i);
		dest.writef ("\t\t\t\t\t<vcount>");
		for(size_t j = 0; j < mesh.tris.size(); j++) {
			dest.writef("3 ");
		}
		for(size_t j = 0; j < mesh.quads.size(); j++) {
			dest.writef("4 ");
		}
		if(mesh.tris.size() + mesh.quads.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</vcount>");
		dest.writef ("\t\t\t\t\t<p>");
		for(const TriFace& tri : mesh.tris) {
			dest.writef("%d %d %d ", tri.v0, tri.v1, tri.v2);
		}
		for(const QuadFace& quad : mesh.quads) {
			dest.writef("%d %d %d %d ", quad.v0, quad.v1, quad.v2, quad.v3);
		}
		if(mesh.tris.size() + mesh.quads.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</p>");
		dest.writelf("\t\t\t\t</polylist>");
		dest.writelf("\t\t\t</mesh>");
		dest.writelf("\t\t</geometry>");
	}
	dest.writelf("\t</library_geometries>");
}

template <typename Value>
static void write_float_array(OutBuffer dest, const std::vector<Value> src, s32 mesh, const char* name) {
	dest.writef ("\t\t\t\t\t<float_array id=\"mesh_%d_%s_array\" count=\"%d\">", mesh, name, src.size() * Value::length());
	for(const Value& val : src) {
		for(s32 i = 0; i < Value::length(); i++) {
			dest.writef("%f ", val[i]);
		}
	}
	if(src.size() > 0) {
		dest.vec.resize(dest.vec.size() - 1);
	}
	dest.writelf("</float_array>");
}

static void write_library_visual_scenes(OutBuffer dest, const DaeScene& scene) {
	dest.writelf("\t<library_visual_scenes>");
	dest.writelf("\t\t<visual_scene id=\"scene\">");
	for(const DaeNode& node : scene.nodes) {
		write_node(dest, node);
	}
	dest.writelf("\t\t</visual_scene>");
	dest.writelf("\t</library_visual_scenes>");
}

static void write_node(OutBuffer dest, const DaeNode& node) {
	dest.writelf("\t\t\t<node id=\"%s\">", node.name.c_str());
	if(node.translate.has_value()) {
		dest.writelf("\t\t\t\t<translate>%f %f %f</translate>",
			node.translate->x, node.translate->y, node.translate->z);
	}
	dest.writelf("\t\t\t\t<instance_geometry url=\"#mesh_%d\">", node.mesh);
	dest.writelf("\t\t\t\t\t<bind_material>");
	dest.writelf("\t\t\t\t\t\t<technique_common>");
	dest.writelf("\t\t\t\t\t\t\t<instance_material symbol=\"defaultMaterial\" target=\"#material\">");
	dest.writelf("\t\t\t\t\t\t\t</instance_material>");
	dest.writelf("\t\t\t\t\t\t</technique_common>");
	dest.writelf("\t\t\t\t\t</bind_material>");
	dest.writelf("\t\t\t\t</instance_geometry>", node.mesh);
	dest.writelf("\t\t\t</node>");
}

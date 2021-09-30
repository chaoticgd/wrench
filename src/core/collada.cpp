
#include "collada.h"

ColladaScene mesh_to_dae(Mesh mesh) {
	ColladaNode node;
	node.name = "node";
	node.mesh = 0;
	
	ColladaScene scene;
	scene.nodes.push_back(node);
	scene.meshes.push_back(mesh);
	return scene;
}

ColladaScene import_dae(std::vector<u8> src) {
	return {};
}

static void write_asset_metadata(OutBuffer dest);
static void write_library_effects(OutBuffer dest);
static void write_library_materials(OutBuffer dest);
static void write_library_geometries(OutBuffer dest, const std::vector<Mesh>& meshes);
static void write_library_visual_scenes(OutBuffer dest, const ColladaScene& scene);
static void write_node(OutBuffer dest, const ColladaNode& node);

std::vector<u8> write_collada(const ColladaScene& scene) {
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
		dest.writelf("\t\t\t\t<source id=\"mesh_%d_source\">", i);
		
		s32 components = 3;
		if(mesh.has_uvs) {
			components += 2;
		}
		dest.writesf("\t\t\t\t\t<float_array id=\"mesh_%d_array\" count=\"%d\">", i, components * mesh.vertices.size());
		if(mesh.has_uvs) {
			for(const Vertex& v : mesh.vertices) {
				dest.writesf("%f %f %f %f %f ", v.pos.x, v.pos.y, v.pos.z, v.uv.x, v.uv.y);
			}
		} else {
			for(const Vertex& v : mesh.vertices) {
				dest.writesf("%f %f %f ", v.pos.x, v.pos.y, v.pos.z);
			}
		}
		if(mesh.vertices.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</float_array>");
		
		dest.writelf("\t\t\t\t\t<technique_common>");
		dest.writelf("\t\t\t\t\t\t<accessor count=\"%d\" offset=\"0\" source=\"mesh_%d_array\" stride=\"%d\">", mesh.vertices.size(), i, components);
		dest.writelf("\t\t\t\t\t\t\t<param name=\"X\" type=\"float\" />");
		dest.writelf("\t\t\t\t\t\t\t<param name=\"Y\" type=\"float\" />");
		dest.writelf("\t\t\t\t\t\t\t<param name=\"Z\" type=\"float\" />");
		dest.writelf("\t\t\t\t\t\t</accessor>");
		dest.writelf("\t\t\t\t\t</technique_common>");
		dest.writelf("\t\t\t\t</source>");
		if(mesh.has_uvs) {
			dest.writelf("\t\t\t\t<source id=\"mesh_%d_uvs\">", i);
			dest.writelf("\t\t\t\t\t<technique_common>");
			dest.writelf("\t\t\t\t\t\t<accessor count=\"%d\" offset=\"3\" source=\"mesh_%d_array\" stride=\"%d\">", mesh.vertices.size(), i, components);
			dest.writelf("\t\t\t\t\t\t\t<param name=\"U\" type=\"float\" />");
			dest.writelf("\t\t\t\t\t\t\t<param name=\"V\" type=\"float\" />");
			dest.writelf("\t\t\t\t\t\t</accessor>");
			dest.writelf("\t\t\t\t\t</technique_common>");
			dest.writelf("\t\t\t\t</source>");
		}
		dest.writelf("\t\t\t\t<vertices id=\"mesh_%d_vertices\">", i);
		dest.writelf("\t\t\t\t\t<input semantic=\"POSITION\" source=\"#mesh_%d_source\" />", i);
		dest.writelf("\t\t\t\t</vertices>");
		dest.writelf("\t\t\t\t<polylist count=\"%d\" material=\"defaultMaterial\">", mesh.tris.size() + mesh.quads.size());
		dest.writelf("\t\t\t\t\t<input offset=\"0\" semantic=\"VERTEX\" source=\"#mesh_%d_vertices\" />", i);
		dest.writesf("\t\t\t\t\t<vcount>");
		for(size_t j = 0; j < mesh.tris.size(); j++) {
			dest.writesf("3 ");
		}
		for(size_t j = 0; j < mesh.quads.size(); j++) {
			dest.writesf("4 ");
		}
		if(mesh.tris.size() + mesh.quads.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</vcount>");
		dest.writesf("\t\t\t\t\t<p>");
		for(const TriFace& tri : mesh.tris) {
			dest.writesf("%d %d %d ", tri.v0, tri.v1, tri.v2);
		}
		for(const QuadFace& quad : mesh.quads) {
			dest.writesf("%d %d %d %d ", quad.v0, quad.v1, quad.v2, quad.v3);
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

static void write_library_visual_scenes(OutBuffer dest, const ColladaScene& scene) {
	dest.writelf("\t<library_visual_scenes>");
	dest.writelf("\t\t<visual_scene id=\"scene\">");
	for(const ColladaNode& node : scene.nodes) {
		write_node(dest, node);
	}
	dest.writelf("\t\t</visual_scene>");
	dest.writelf("\t</library_visual_scenes>");
}

static void write_node(OutBuffer dest, const ColladaNode& node) {
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
	dest.writelf("\t\t\t\t</instance_geometry>");
	dest.writelf("\t\t\t</node>");
}

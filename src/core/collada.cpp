
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
static void write_library_images(OutBuffer dest, const std::vector<std::string>& texture_paths);
static void write_library_effects(OutBuffer dest, s32 texture_count);
static void write_library_materials(OutBuffer dest, s32 texture_count);
static void write_library_geometries(OutBuffer dest, const std::vector<Mesh>& meshes);
static void write_library_visual_scenes(OutBuffer dest, const ColladaScene& scene, s32 texture_count);
static void write_node(OutBuffer dest, const ColladaNode& node, s32 texture_count);

std::vector<u8> write_collada(const ColladaScene& scene, const std::vector<std::string>& texture_paths) {
	std::vector<u8> vec;
	OutBuffer dest(vec);
	dest.writelf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	dest.writelf("<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">");
	write_asset_metadata(dest);
	if(texture_paths.size() > 0) {
		write_library_images(dest, texture_paths);
	}
	write_library_materials(dest, (s32) texture_paths.size());
	write_library_effects(dest, (s32) texture_paths.size());
	write_library_geometries(dest, scene.meshes);
	write_library_visual_scenes(dest, scene, (s32) texture_paths.size());
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
	dest.writelf("\t\t<unit name=\"meter\" meter=\"1\"/>");
	dest.writelf("\t\t<up_axis>Z_UP</up_axis>");
	dest.writelf("\t</asset>");
}

static void write_library_images(OutBuffer dest, const std::vector<std::string>& texture_paths) {
	dest.writelf("\t<library_images>");
	for(s32 i = 0; i < (s32) texture_paths.size(); i++) {
		dest.writelf("\t\t<image id=\"texture_%d\">", i);
		dest.writesf("\t\t\t<init_from>");
		const std::string& path = texture_paths[i];
		dest.vec.insert(dest.vec.end(), path.begin(), path.end());
		dest.writelf("</init_from>");
		dest.writelf("\t\t</image>");
	}
	dest.writelf("\t</library_images>");
}

static void write_library_effects(OutBuffer dest, s32 texture_count) {
	dest.writelf("\t<library_effects>");
	for(s32 i = 0; i < texture_count; i++) {
		dest.writelf("\t\t<effect id=\"effect_%d\">", i);
		dest.writelf("\t\t\t<profile_COMMON>");
		dest.writelf(4, "<newparam sid=\"effect_%d_surface\">", i);
		dest.writelf(4, "\t<surface type=\"2D\">");
		dest.writelf(4, "\t\t<init_from>texture_%d</init_from>", i);
		dest.writelf(4, "\t\t<format>A8R8G8B8</format>");
		dest.writelf(4, "\t</surface>");
		dest.writelf(4, "</newparam>");
		dest.writelf(4, "<newparam sid=\"effect_%d_sampler\">", i);
		dest.writelf(4, "\t<sampler2D>");
		dest.writelf(4, "\t\t<source>effect_%d_surface</source>", i);
		dest.writelf(4, "\t\t<minfilter>LINEAR_MIPMAP_LINEAR</minfilter>");
		dest.writelf(4, "\t\t<magfilter>LINEAR</magfilter>");
		dest.writelf(4, "\t</sampler2D>");
		dest.writelf(4, "</newparam>");
		dest.writelf(4, "<technique sid=\"common\">");
		dest.writelf(4, "\t<lambert>");
		dest.writelf(4, "\t\t<diffuse>");
		dest.writelf(4, "\t\t\t<texture texture=\"effect_%d_sampler\" texcoord=\"texture_%d_coord\"/>", i, i);
		dest.writelf(4, "\t\t</diffuse>");
		dest.writelf(4, "\t</lambert>");
		dest.writelf(4, "</technique>");
		dest.writelf("\t\t\t</profile_COMMON>");
		dest.writelf("\t\t</effect>");
	}
	dest.writelf("\t</library_effects>");
}

static void write_library_materials(OutBuffer dest, s32 texture_count) {
	dest.writelf("\t<library_materials>");
	for(s32 i = 0; i < texture_count; i++) {
		dest.writelf("\t\t<material id=\"material_%d\">", i);
		dest.writelf("\t\t\t<instance_effect url=\"#effect_%d\"/>", i);
		dest.writelf("\t\t</material>");
	}
	dest.writelf("\t</library_materials>");
}

static void write_library_geometries(OutBuffer dest, const std::vector<Mesh>& meshes) {
	dest.writelf("\t<library_geometries>");
	for(size_t i = 0; i < meshes.size(); i++) {
		const Mesh& mesh = meshes[i];
		dest.writelf("\t\t<geometry id=\"mesh_%d\">", i);
		dest.writelf("\t\t\t<mesh>");
		
		dest.writelf(4, "<source id=\"mesh_%d_positions\">", i);
		dest.writesf(4, "\t<float_array id=\"mesh_%d_positions_array\" count=\"%d\">", i, 3 * mesh.vertices.size());
		for(const Vertex& v : mesh.vertices) {
			dest.writesf("%f %f %f ", v.pos.x, v.pos.y, v.pos.z);
		}
		if(mesh.vertices.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</float_array>");
		dest.writelf(4, "\t<technique_common>");
		dest.writelf(4, "\t\t<accessor count=\"%d\" offset=\"0\" source=\"#mesh_%d_positions_array\" stride=\"3\">", mesh.vertices.size(), i);
		dest.writelf(4, "\t\t\t<param name=\"X\" type=\"float\"/>");
		dest.writelf(4, "\t\t\t<param name=\"Y\" type=\"float\"/>");
		dest.writelf(4, "\t\t\t<param name=\"Z\" type=\"float\"/>");
		dest.writelf(4, "\t\t</accessor>");
		dest.writelf(4, "\t</technique_common>");
		dest.writelf(4, "</source>");
		if(mesh.flags & MESH_HAS_TEX_COORDS) {
			dest.writelf(4, "<source id=\"mesh_%d_texcoords\">", i);
			dest.writesf(4, "\t<float_array id=\"mesh_%d_texcoords_array\" count=\"%d\">", i, 2 * mesh.vertices.size());
			for(const Vertex& v : mesh.vertices) {
				dest.writesf("%f %f ", v.tex_coord.x, v.tex_coord.y);
			}
			if(mesh.vertices.size() > 0) {
				dest.vec.resize(dest.vec.size() - 1);
			}
			dest.writelf("</float_array>");
			dest.writelf(4, "\t<technique_common>");
			dest.writelf(4, "\t\t<accessor count=\"%d\" offset=\"0\" source=\"#mesh_%d_texcoords_array\" stride=\"2\">", mesh.vertices.size(), i);
			dest.writelf(4, "\t\t\t<param name=\"S\" type=\"float\"/>");
			dest.writelf(4, "\t\t\t<param name=\"T\" type=\"float\"/>");
			dest.writelf(4, "\t\t</accessor>");
			dest.writelf(4, "\t</technique_common>");
			dest.writelf(4, "</source>");
		}
		dest.writelf(4, "<vertices id=\"mesh_%d_vertices\">", i);
		dest.writelf(4, "\t<input semantic=\"POSITION\" source=\"#mesh_%d_positions\"/>", i);
		dest.writelf(4, "</vertices>");
		if(mesh.flags & MESH_HAS_QUADS) {
			for(const SubMesh& submesh : mesh.submeshes) {
				dest.writelf(4, "<polylist count=\"%d\" material=\"material_symbol_%d\">", submesh.faces.size(), submesh.texture);
				dest.writelf(4, "\t<input offset=\"0\" semantic=\"VERTEX\" source=\"#mesh_%d_vertices\"/>", i);
				if(mesh.flags & MESH_HAS_TEX_COORDS) {
					dest.writelf(4, "\t<input offset=\"0\" semantic=\"TEXCOORD\" source=\"#mesh_%d_texcoords\" set=\"0\"/>", i);
				}
				dest.writesf(4, "\t<vcount>");
				for(const Face& face : submesh.faces) {
					if(face.v3 > -1) {
						dest.writesf("4 ");
					} else {
						dest.writesf("3 ");
					}
				}
				if(submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</vcount>");
				dest.writesf(4, "\t<p>");
				for(const Face& face : submesh.faces) {
					if(face.v3 > -1) {
						dest.writesf("%d %d %d %d ", face.v0, face.v1, face.v2, face.v3);
					} else {
						dest.writesf("%d %d %d ", face.v0, face.v1, face.v2);
					}
				}
				if(submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</p>");
				dest.writelf(4, "</polylist>");
			}
		} else {
			for(const SubMesh& submesh : mesh.submeshes) {
				dest.writelf(4, "<triangles count=\"%d\" material=\"material_symbol_%d\">", submesh.faces.size(), submesh.texture);
				dest.writelf(4, "\t<input offset=\"0\" semantic=\"VERTEX\" source=\"#mesh_%d_vertices\"/>", i);
				if(mesh.flags & MESH_HAS_TEX_COORDS) {
					dest.writelf(4, "\t<input offset=\"0\" semantic=\"TEXCOORD\" source=\"#mesh_%d_texcoords\" set=\"0\"/>", i);
				}
				dest.writesf(4, "\t<p>");
				for(const Face& face : submesh.faces) {
					dest.writesf("%d %d %d ", face.v0, face.v1, face.v2);
				}
				if(submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</p>");
				dest.writelf(4, "</triangles>");
			}
		}
		dest.writelf("\t\t\t</mesh>");
		dest.writelf("\t\t</geometry>");
	}
	dest.writelf("\t</library_geometries>");
}

static void write_library_visual_scenes(OutBuffer dest, const ColladaScene& scene, s32 texture_count) {
	dest.writelf("\t<library_visual_scenes>");
	dest.writelf("\t\t<visual_scene id=\"scene\">");
	for(const ColladaNode& node : scene.nodes) {
		write_node(dest, node, texture_count);
	}
	dest.writelf("\t\t</visual_scene>");
	dest.writelf("\t</library_visual_scenes>");
}

static void write_node(OutBuffer dest, const ColladaNode& node, s32 texture_count) {
	dest.writelf("\t\t\t<node id=\"%s\">", node.name.c_str());
	if(node.translate.has_value()) {
		dest.writelf("\t\t\t\t<translate>%f %f %f</translate>",
			node.translate->x, node.translate->y, node.translate->z);
	}
	dest.writelf(4, "<instance_geometry url=\"#mesh_%d\">", node.mesh);
	dest.writelf(4, "\t<bind_material>");
	dest.writelf(4, "\t\t<technique_common>");
	for(s32 i = 0; i < texture_count; i++) {
		dest.writelf(7, "<instance_material symbol=\"material_symbol_%d\" target=\"#material_%d\">", i, i);
		dest.writelf(7, "\t<bind_vertex_input semantic=\"texture_%d_coord\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>", i);
		dest.writelf(7, "</instance_material>");
	}
	dest.writelf(4, "\t\t</technique_common>");
	dest.writelf(4, "\t</bind_material>");
	dest.writelf(4, "</instance_geometry>");
	dest.writelf("\t\t\t</node>");
}

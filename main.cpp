#include <fstream>
#include <random>
#include <float.h>
#include "sphere.h"
#include "hitable_list.h"
#include "camera.h"
#include "material.h"
#include "bvh.h"
#include "aarect.h"
#include "box.h"
#include "triangle.h"

// lib for .PLY
#include "tinyply\source\tinyply.h"

//needed for rng
#include "perlin.h"
#include "constant_medium.h"

// needed for image textures
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// see how long we took
#include <chrono>

#include <memory>

const float _pi = 3.14159265358979f;
std::default_random_engine generator;
std::uniform_real_distribution<float> drand(0, 1.0);

float get_rand() {
	return drand(generator);
}

vec3 color(const ray& r, hitable *world, int depth) {
	hit_record rec;
	if (world->hit(r, 0.001f, FLT_MAX, rec)) {
		ray scattered;
		vec3 attenuation;
		vec3 emitted = rec.mat_ptr->emitted(rec.u, rec.v, rec.p);
		if (depth < 50 && rec.mat_ptr->scatter(r, rec, attenuation, scattered))
			return emitted + attenuation*color(scattered, world, depth + 1);
		else
			return emitted;
	}
	else
	{
		/*vec3 unit_direction = unit_vector(r.direction());
		float t = 0.5 * (unit_direction.y() + 1.0);
		return (1.0 - t) * vec3(1.0, 1.0, 1.0) + t * vec3(0.5, 0.7, 1.0);*/
		return vec3(0, 0, 0);
	}
}

hitable *random_scene() {
	int n = 50000;
	hitable **list = new hitable*[n + 1];
	texture *checker = new checker_texture(new constant_texture(vec3(0.2f, 0.3f, 0.1f)), new constant_texture(vec3(0.9f, 0.9f, 0.9f)));
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(checker));
	int i = 1;
	for (int a = -10; a < 10; a++) {
		for (int b = -10; b < 10; b++) {
			float choose_mat = get_rand();
			vec3 center(a + 0.9f + get_rand(), 0.2f, b + 0.9f*get_rand());
			if ((center - vec3(4, 0.2f, 0)).length() > 0.9f) {
				if (choose_mat < 0.8f) { // diffuse
					list[i++] = new moving_sphere(center, center + vec3(0, 0.5f*get_rand(), 0),
						0.0f, 1.0f, 0.2f, new lambertian(new constant_texture(
							vec3(get_rand()*get_rand(),
								get_rand()*get_rand(),
								get_rand()*get_rand()))));
				}
				else if (choose_mat < 0.95f) { // metal
					list[i++] = new sphere(center, 0.2f, new metal(
						vec3(0.5f*(1 + get_rand()),
							0.5f*(1 + get_rand()),
							0.5f*(1 + get_rand())),
						0.5f*get_rand()));
				}
				else { // glass
					list[i++] = new sphere(center, 0.2f, new dielectric(1.5f));
				}
			}
		}
	}

	list[i++] = new sphere(vec3(0, 1, 0), 1.0, new dielectric(1.5f));
	//list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new lambertian(new constant_texture(vec3(0.4f, 0.2f, 0.1f))));
	list[i++] = new sphere(vec3(-4, 1, 0), 1.0, new diffuse_light(new constant_texture(vec3(4, 4, 4))));
	list[i++] = new sphere(vec3(4, 1, 0), 1.0, new metal(vec3(0.7f, 0.6f, 0.5f), 0.0f));

	//return new hitable_list(list, i);
	return new bvh_node(list, i, 0.0, 1.0);
}

hitable *two_spheres() {
	texture *checker = new checker_texture(new constant_texture(vec3(0.2f, 0.3f, 0.1f)), new constant_texture(vec3(0.9f, 0.9f, 0.9f)));
	hitable **list = new hitable*[2];
	list[0] = new sphere(vec3(0, -10, 0), 10, new lambertian(checker));
	list[1] = new sphere(vec3(0, 10, 0), 10, new lambertian(checker));

	return new bvh_node(list, 2, 0.0, 1.0);
}

hitable *two_perlin_spheres() {
	texture *pertext = new noise_texture(4);
	hitable **list = new hitable*[2];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, new lambertian(pertext));
	list[1] = new sphere(vec3(0, 2, 0), 2, new lambertian(pertext));
	return new bvh_node(list, 2, 0.0, 1.0);
}

hitable *earth() {
	int nx, ny, nn;
	unsigned char *tex_data = stbi_load("earthmap.jpg", &nx, &ny, &nn, 0);
	material *mat = new lambertian(new image_texture(tex_data, nx, ny));

	return new sphere(vec3(0, 0, 0), 2, mat);
}

hitable *simple_light() {
	material *pertext = new lambertian(new noise_texture(4));
	material *red = new lambertian(new constant_texture(vec3(0.65f, 0.05f, 0.05f)));;
	material *green = new lambertian(new constant_texture(vec3(0.12f, 0.45f, 0.15f)));
	material *checker = new lambertian(new checker_texture(new constant_texture(vec3(0.2f, 0.3f, 0.1f)), new constant_texture(vec3(0.9f, 0.9f, 0.9f))));
	material *blue = new lambertian(new constant_texture(vec3(0.05f, 0.05f, 0.65f)));
	hitable **list = new hitable*[4];
	list[0] = new sphere(vec3(0, -1000, 0), 1000, pertext);
	list[1] = new sphere(vec3(0, 2, 0), 2, pertext);
	list[2] = new sphere(vec3(0, 7, 0), 2, new diffuse_light(new constant_texture(vec3(4, 4, 4))));
	list[3] = new xy_rect(3, 5, 1, 3, -2, new diffuse_light(new constant_texture(vec3(4, 4, 4))));
	return new bvh_node(list, 4, 0.0, 1.0);
}

hitable *cornell_box() {
	hitable **list = new hitable*[6];
	int i = 0;
	material *red = new lambertian(new constant_texture(vec3(0.65f, 0.05f, 0.05f)));;
	material *white = new lambertian(new constant_texture(vec3(0.73f, 0.73f, 0.73f)));
	material *green = new lambertian(new constant_texture(vec3(0.12f, 0.45f, 0.15f)));
	material *light = new diffuse_light(new constant_texture(vec3(4, 4, 4)));
	list[i++] = new flip_normals(new yz_rect(0, 555, 0, 555, 555, green)); // Left wall
	list[i++] = new yz_rect(0, 555, 0, 555, 0, red); // Right wall
	//list[i++] = new xz_rect(213, 343, 227, 332, 554, light);
	list[i++] = new xz_rect(113, 443, 127, 432, 554, light); // bigger light
	list[i++] = new flip_normals(new xz_rect(0, 555, 0, 555, 555, white)); // Roof
	list[i++] = new xz_rect(0, 555, 0, 555, 0, white); // Floor
	list[i++] = new flip_normals(new xy_rect(0, 555, 0, 555, 555, white)); // Back
	list[i++] = new box(vec3(130, 0, 65), vec3(295, 165, 230), white); // front box
	list[i++] = new box(vec3(265, 0, 295), vec3(430, 330, 460), white); // rear box
	list[i++] = new triangle(vec3(130, 130, 260), vec3(430, 130, 260), vec3(300, 330, 260), green);
	//list[i++] = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 165, 165), white), -18), vec3(130, 0, 65)); // front box
	//list[i++] = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 330, 165), white), 15), vec3(265, 0, 295)); // rear box
	//list[i++] = new rotate_y(new box(vec3(130, 0, 65), vec3(295, 165, 230), white), -18); // front box
	//list[i++] = new rotate_y(new box(vec3(265, 0, 295), vec3(430, 330, 460), white), 15); // rear box
	return new bvh_node(list, i, 0.0, 1.0);
}

hitable *cornell_smoke() {
	hitable **list = new hitable*[8];
	int i = 0;
	material *red = new lambertian(new constant_texture(vec3(0.65f, 0.05f, 0.05f)));;
	material *white = new lambertian(new constant_texture(vec3(0.73f, 0.73f, 0.73f)));
	material *green = new lambertian(new constant_texture(vec3(0.12f, 0.45f, 0.15f)));
	material *light = new diffuse_light(new constant_texture(vec3(4, 4, 4)));
	list[i++] = new flip_normals(new yz_rect(0, 555, 0, 555, 555, green)); // Left wall
	list[i++] = new yz_rect(0, 555, 0, 555, 0, red); // Right wall
	list[i++] = new xz_rect(113, 443, 127, 432, 554, light);
	list[i++] = new flip_normals(new xz_rect(0, 555, 0, 555, 555, white)); // Roof
	list[i++] = new xz_rect(0, 555, 0, 555, 0, white); // Floor
	list[i++] = new flip_normals(new xy_rect(0, 555, 0, 555, 555, white)); // Back
	hitable *b1 = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 165, 165), white), -18), vec3(130, 0, 65)); // front box
	hitable *b2 = new translate(new rotate_y(new box(vec3(0, 0, 0), vec3(165, 330, 165), white), 15), vec3(265, 0, 295)); // rear box
	list[i++] = new constant_medium(b1, 0.01, new constant_texture(vec3(1.0, 1.0, 1.0)));
	list[i++] = new constant_medium(b2, 0.01, new constant_texture(vec3(0.0, 0.0, 0.0)));
	return new bvh_node(list, i, 0.0, 1.0);
}

hitable *final() {
	int nb = 20;
	hitable **list = new hitable*[30];
	hitable **boxlist = new hitable*[10000];
	hitable **boxlist2 = new hitable*[10000];
	material *white = new lambertian(new constant_texture(vec3(0.73, 0.73, 0.73)));
	material *ground = new lambertian(new constant_texture(vec3(0.48, 0.83, 0.53)));
	int b = 0;
	for (int i = 0; i < nb; i++) {
		for (int j = 0; j < nb; j++) {
			float w = 100;
			float x0 = -1000 + i*w;
			float z0 = -1000 + j*w;
			float y0 = 0;
			float x1 = x0 + w;
			float y1 = 100 * (get_rand() + 0.01);
			float z1 = z0 + w;
			boxlist[b++] = new box(vec3(x0, y0, z0), vec3(x1, y1, z1), ground);
		}
	}
	int l = 0;
	list[l++] = new bvh_node(boxlist, b, 0, 1);
	material *light = new diffuse_light(new constant_texture(vec3(7, 7, 7)));
	list[l++] = new xz_rect(123, 423, 147, 412, 554, light);
	vec3 center(400, 400, 200);
	list[l++] = new moving_sphere(center, center + vec3(30, 0, 0), 0, 1, 50, new lambertian(new constant_texture(vec3(0.7, 0.3, 0.1))));
	list[l++] = new sphere(vec3(260, 150, 45), 50, new dielectric(1.5));
	list[l++] = new sphere(vec3(0, 150, 145), 50, new metal(vec3(0.8, 0.8, 0.9), 10.0));
	hitable *boundary = new sphere(vec3(360, 150, 145), 70, new dielectric(1.5));
	list[l++] = boundary;
	list[l++] = new constant_medium(boundary, 0.2, new constant_texture(vec3(0.2, 0.4, 0.9)));
	boundary = new sphere(vec3(0, 0, 0), 5000, new dielectric(1.5));
	list[l++] = new constant_medium(boundary, 0.0001, new constant_texture(vec3(1.0, 1.0, 1.0)));
	int nx, ny, nn;
	unsigned char *tex_data = stbi_load("earthmap.jpg", &nx, &ny, &nn, 0);
	material *emat = new lambertian(new image_texture(tex_data, nx, ny));
	list[l++] = new sphere(vec3(400, 200, 400), 100, emat);
	texture *pertext = new noise_texture(0.1);
	list[l++] = new sphere(vec3(220, 280, 300), 80, new lambertian(pertext));
	int ns = 1000;
	for (int j = 0; j < ns; j++) {
		boxlist2[j] = new sphere(vec3(165 * get_rand(), 165 * get_rand(), 165 * get_rand()), 10, white);
	}
	list[l++] = new translate(new rotate_y(new bvh_node(boxlist2, ns, 0.0, 1.0), 15), vec3(-100, 270, 395));
	return new bvh_node(list, l, 0, 1);
}

hitable *triangles() {
	int i = 0;
	material *white = new lambertian(new constant_texture(vec3(0.73, 0.73, 0.73)));
	texture *pertext = new noise_texture(0.5);
	int nx, ny, nn;
	unsigned char *tex_data = stbi_load("earthmap.jpg", &nx, &ny, &nn, 0);
	material *emat = new lambertian(new image_texture(tex_data, nx, ny));
	hitable **list = new hitable*[3];
	//list[i++] = new sphere(vec3(0, 0, -1), 0.5f, new lambertian(new constant_texture(vec3(0.1f, 0.2f, 0.5f)))); // Blue middle
	vec3 a = vec3(-2, 0, -1);
	vec3 b = vec3(2, 0, -1);
	vec3 c = vec3(0, 2, -1);
	hitable *tri = new triangle(a, b, c, new lambertian(new constant_texture(vec3(0.0, 0.0, 1.0))));
	// z axis seems backwards, more positive should be toward camera
	a = vec3(0, 2, -1);
	b = vec3(2, 0, -1);
	c = vec3(2, 1, -2);
	hitable *tri2 = new triangle(a, b, c, new lambertian(new constant_texture(vec3(0.3, 0.9, 0.7))));
	hitable *marble = new sphere(vec3(0, 1.7, -3), 1.5f, new lambertian(pertext));
	//b = c;
	//c = vec3(1, 1, -3);
	//hitable *tri2 = new triangle(a, b, c, new metal(vec3(0.2, 0.6, 0.6), 10));
	a = vec3(-2, 2, -3);
	b = vec3(2, 2, -3);
	c = vec3(0, 0, -3);
	hitable *tri3 = new triangle(a, b, c, new lambertian(new constant_texture(vec3(1.0, 0.0, 0.0))));
	aabb bbox;
	tri->bounding_box(0, 1, bbox);
 	hitable *bbox_test = new box(bbox.min(), bbox.max(), white);
	tri2->bounding_box(0, 1, bbox);
	hitable *bbox_test2 = new box(bbox.min(), bbox.max(), new lambertian(pertext));
	list[i++] = tri;
	//list[i++] = marble;
	list[i++] = tri2;
	//list[i++] = tri3;
	//list[i++] = bbox_test;
	//list[i++] = bbox_test2;
	//list[i++] = new constant_medium(bbox_test, 1.0, new constant_texture(vec3(1.0, 1.0, 1.0)));
	list[i++] = new sphere(vec3(0, -100.5f, -1), 100, new lambertian(new constant_texture(vec3(0.8f, 0.8f, 0.0f)))); // Giant green that acts as ground
	//list[i++] = new sphere(vec3(1, 0, -1), 0.5f, new metal(vec3(0.8f, 0.6f, 0.2f))); // Metal right
	//hitable *boundary = new sphere(vec3(-1, 0, -1), 0.5f, new dielectric(1.5f));
	//list[i++] = new constant_medium(boundary, 0.95f, new constant_texture(vec3(1.0, 1.0, 1.0)));
	//list[i++] = new sphere(vec3(-1, 0, -1), 0.5f, new dielectric(1.5f)); // These two act as a sort of glass bubble
	//list[i++] = new sphere(vec3(-1, 0, -1), -0.45f, new dielectric(1.5f)); // Only work together though?
	
	return new bvh_node(list, i, 0, 1);
}

hitable *ply_test() {
	int count = 0;
	hitable **list = new hitable*[0];
	std::string filename = "tinyply\\assets\\icosahedron.ply";
	//std::string filename = "tinyply\\assets\\sofa.ply";
	//filename = "bunny.tar\\bunny\\bunny\\reconstruction\\bun_zipper_res2.ply";
	//filename = "dragon_recon.tar\\dragon_recon\\dragon_recon\\dragon_vrip_res4.ply";
	//import our PLY model
	try
	{
		std::ifstream ss(filename, std::ios::binary);

		// Parse the header
		tinyply::PlyFile file(ss);

		for (auto e : file.get_elements())
		{
			std::cout << "element - " << e.name << " (" << e.size << ")" << std::endl;
			for (auto p : e.properties)
			{
				std::cout << "\tproperty - " << p.name << " (" << tinyply::PropertyTable[p.propertyType].str << ")" << std::endl;
			}
		}
		std::cout << std::endl;

		for (auto c : file.comments)
		{
			std::cout << "Comment: " << c << std::endl;
		}

		std::vector<float> verts;
		std::vector<float> norms;
		std::vector<uint8_t> colors;
		std::vector<uint32_t> faces;
		std::vector<float> uvCoords;

		uint32_t vertexCount, normalCount, colorCount, faceCount, faceTexcoordCount, faceColorCount;
		vertexCount = normalCount = colorCount = faceCount = faceTexcoordCount = faceColorCount = 0;

		vertexCount = file.request_properties_from_element("vertex", { "x", "y", "z" }, verts);
		normalCount = file.request_properties_from_element("vertex", { "nx", "ny", "nz" }, norms);
		colorCount = file.request_properties_from_element("vertex", { "red", "green", "blue", "alpha" }, colors);

		faceCount = file.request_properties_from_element("face", { "vertex_indices" }, faces, 3);
		faceTexcoordCount = file.request_properties_from_element("face", { "texcoord" }, uvCoords, 6);

		file.read(ss);

		std::cout << "\tRead " << verts.size() << " total vertices (" << vertexCount << " properties)." << std::endl;
		std::cout << "\tRead " << norms.size() << " total normals (" << normalCount << " properties)." << std::endl;
		std::cout << "\tRead " << colors.size() << " total vertex colors (" << colorCount << " properties)." << std::endl;
		std::cout << "\tRead " << faces.size() << " total faces (triangles) (" << faceCount << " properties)." << std::endl;
		std::cout << "\tRead " << uvCoords.size() << " total texcoords (" << faceTexcoordCount << " properties)." << std::endl;
		std::cout << "\n";

		std::vector<vec3> points;
		std::vector<triangle> tris;
		hitable **list = new hitable*[faceCount]; // declare our array with the proper size from the ply, instead of guessing
		
		for (int i = 0; i < faces.size(); i++) {
			int offset = faces[i] * 3;
			vec3 temp = vec3(verts[offset], verts[offset + 1], verts[offset + 2]);

			points.push_back(temp);
		}
		//list[count++] = new sphere(vec3(0, -100.5f, -1), 100, new diffuse_light(new constant_texture(vec3(4.0f, 4.0f, 4.0f)))); // Giant green that acts as ground
		material *mat = new metal(vec3(0.8, 0.5, 0.2), 0.5f);
		//material *mat = new lambertian(new constant_texture(vec3(0.5f, 0.1f, 0.5f)));
		//material *mat = new diffuse_light(new constant_texture(vec3(4.0f, 4.0f, 4.0f)));
		// now we've got our points out, we can make triangles out of them.
		for (int i = 0; i < points.size(); i+=3) {
			list[count++] = new triangle(points[i], points[i + 1], points[i + 2], mat);
		}
		// holy shit it fucking works!
	}
	catch (const std::exception& e)
	{
		std::cerr << "Caught exception: " << e.what() << std::endl;
	}


	return new bvh_node(list, count, 0, 1);
}

int main() {
	auto t_start = std::chrono::high_resolution_clock::now();
	// Seed RNG
	generator.seed(std::random_device()());
	int nx = 400;
	int ny = 200;
	int ns = 1024;
	
	//std::ofstream ost{ "scene.ppm" };
	//ost << "P3\n" << nx << " " << ny << "\n255\n";
	const int NUM_SPHERES = 5;
	hitable *list[NUM_SPHERES];
	list[0] = new sphere(vec3(0, 0, -1), 0.5f, new lambertian(new constant_texture(vec3(0.1f, 0.2f, 0.5f)))); // Blue middle
	list[1] = new sphere(vec3(0, -100.5f, -1), 100, new lambertian(new constant_texture(vec3(0.8f, 0.8f, 0.0f)))); // Giant green that acts as ground
	list[2] = new sphere(vec3(1, 0, -1), 0.5f, new metal(vec3(0.8f, 0.6f, 0.2f))); // Metal right
	list[3] = new sphere(vec3(-1, 0, -1), 0.5f, new dielectric(1.5f)); // These two act as a sort of glass bubble
	list[4] = new sphere(vec3(-1, 0, -1), -0.45f, new dielectric(1.5f)); // Only work together though?

	//hitable *world = new hitable_list(list, NUM_SPHERES);
	hitable *world = new bvh_node(list, NUM_SPHERES, 0.0, 1.0);
	world = random_scene();
	//world = two_spheres();
	//world = two_perlin_spheres();
	//world = earth();
	//world = simple_light();
	//world = cornell_box();
	//world = cornell_smoke();
	//world = final();
	//world = triangles();
	//world = ply_test();
	vec3 lookfrom(13, 3, 2);
	//lookfrom = vec3(0, 0.05f, 0.1f);
	//vec3 lookfrom(5, 0, 0.5f); // icosahedron
	vec3 lookat(0, 0, 0);
	float vfov = 50.0f;
	//float dist_to_focus = (lookfrom - lookat).length();
	//float dist_to_focus = 10.0;
	//float aperture = 0.0;
	// cornell box
	//vec3 lookfrom(278, 278, -800);
	//vec3 lookat(278, 278, 0);
	// "Final" scene
	//vec3 lookfrom(478, 278, -600);
	//vec3 lookat(278, 278, 0);
	float dist_to_focus = 10.0f;
	float aperture = 0.0f;
	//float vfov = 40.0f;

	camera cam(lookfrom, lookat, vec3(0, 1, 0), vfov, float(nx) / float(ny), aperture, dist_to_focus, 0.0, 1.0);
	char *data = new char[nx * ny * 3]; // buffer in bytes for our output image
	int counter = 0;

	for (int j = ny - 1; j >= 0; j--) {
		for (int i = 0; i < nx; i++) {
			vec3 col(0, 0, 0);
			for (int s = 0; s < ns; s++) {
				float u = float(i + get_rand()) / float(nx);
				float v = float(j + get_rand()) / float(ny);
				ray r = cam.get_ray(u, v);
				vec3 p = r.point_at_parameter(2.0);
				col += color(r, world, 0);
			}

			col /= float(ns);
			col = vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
			int ir = int(255.99*col[0]);
			int ig = int(255.99*col[1]);
			int ib = int(255.99*col[2]);

			data[counter++] = ir;
			data[counter++] = ig;
			data[counter++] = ib;

			// predictably, makes slow as fuck
			// todo: multithread?
			//int percent = ((float)j / (float)ny) * 100;
			//std::cout << "\r" << percent << "% remaining | Current Pos: (" << i << ", " << j << ")";
			//std::cout.flush();

			// ppm output
			//ost << ir << " " << ig << " " << ib << "\n";
		}
	}

	// Lets make an image instead
	stbi_write_png("scene.png", nx, ny, 3, data, 0);

	auto t_end = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::duration<float>>(t_end - t_start).count();
	std::cout << "\nTime elapsed: " << floor(time / 60) << " minutes and " << fmod(time, 60) << " seconds." << std::endl;
	std::cin.get();
}
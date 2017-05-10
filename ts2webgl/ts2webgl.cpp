#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <string>

#include "HTMLheader.h"
#include <bitset>

#include "OpenFileDialog.h"


struct Vertex
{
	int32_t index;
	float x, y, z;
	Vertex(){}
	Vertex(float _x, float _y, float _z) { x = _x; y = _y; z = _z; }
	Vertex(int32_t i, float _x, float _y, float _z) { index = i; x = _x; y = _y; z = _z; }
};

struct Triangle
{
	int32_t index;
	int32_t v1, v2, v3;
	Triangle(int32_t i, int32_t _x, int32_t _y, int32_t _z) { index = i; v1 = _x; v2 = _y; v3 = _z; }
};

struct Color
{
	int r, g, b;
	int32_t hex;
	Color(){}
	Color(int _a, int _b, int _c){ r = _a; g = _b; b = _c; }
};

enum objtype { SURF, PTS, PLINE };
enum geotype {am1UK, SyOK, SyUK, WELL, LINEE, REF, UNDEF};

struct GObj
{
	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;
	int32_t vertexnumber, trianglenumber;
	objtype type;
	geotype geo;
	Color color;
	std::string name;
	std::string description;
};

std::vector<GObj> objects;

int main(int argc, char *argv[])
{
	std::cout << "SKUA-GOCAD 2015.5 Surface/Pointset/PLine/Wells to HTML-WEBGL Converter\n";
	std::cout << "============================================\n\n";
	std::cout << "(c) Christian Lehmann 05.05.2017\n\n\n";

	//openDialog();

	filename = "modellierung4";


	std::ifstream ifs(filename.c_str(), std::ifstream::in);
	if (!ifs.good())
	{
		std::cout << "Error loading:" << filename << "; File not found!" << "\n";
		system("PAUSE");
		exit(0);
	}

	std::string line, key, rest;
	std::cout << "Started loading GOCAD ASCII file " << filename << "....";
	int linecounter = 0;

	Color lastcolor;
	
	while (!ifs.eof() && std::getline(ifs, line))
	{
		key = "";
		rest = "";
		std::stringstream stringstream(line);
		stringstream >> key >> std::ws >> rest >> std::ws; // erster teil der zeile in key, dann in rest


		if (key == "GOCAD" && rest == "TSurf")
		{
			GObj new_object;
			new_object.type = SURF;
			new_object.geo = UNDEF;
			new_object.vertexnumber = 0;
			new_object.trianglenumber = 0;
			new_object.color = Color(0, 0, 0); // set standard color (black)
			// jetzt bis Ende Surface durchgehen
			while (!ifs.eof() /*&& std::getline(ifs, line)*/)
			{
				std::getline(ifs, line);
				std::stringstream stringstream2(line);
				// n�chste zeilen durchgehen
				key = "";
				rest = "";
				stringstream2 >> key >> std::ws >> rest >> std::ws;
				if (key == "END") break; // Loop der inneren Schleife abbrechen, n�chstes Objekt f�ngt an
				if (key == "name:")
				{
					new_object.name = rest;
					if (rest.find("am1") != std::string::npos && rest.find("UK") != std::string::npos) new_object.geo = am1UK;
					if (rest.find("Sy") != std::string::npos && rest.find("OK") != std::string::npos) new_object.geo = SyOK;
					if (rest.find("Sy") != std::string::npos && rest.find("UK") != std::string::npos) new_object.geo = SyUK;
				}
				if (key == "*solid*color:")
				{
					float r = 0, g = 0, b = 0;
					std::string dummy;
					std::size_t found = rest.find("#");
					if (found != std::string::npos) // Farbe ist bin�r angegeben
					{
						if (rest[0] == '#') { rest.erase(0, 1); }
						std::string str;
						std::stringstream ss;
						int* rgb = new int[3];

						for (int i = 0; i < 3; i++) {
							str = rest.substr(i * 2, 2);
							ss << std::hex << str;
							ss >> rgb[i];
							ss.clear();
						}

						new_object.color = Color(rgb[0], rgb[1], rgb[2]);

					}
					else // Farbe ist als float angegeben
					{
						stringstream2 >> g >> std::ws >> b >> std::ws >> dummy; // dummy enth�lt die 1 am Ende der Farbzeile
						new_object.color = Color(std::stof(rest.c_str())*255.f, g*255.f, b*255.f);
					}
				}
				if (key == "VRTX")
				{
					std::string rw, hw, teufe;
					stringstream2 >> rw >> std::ws >> hw >> std::ws >> teufe >> std::ws;
					new_object.vertices.push_back(Vertex(new_object.vertexnumber, std::stof(rw), std::stof(hw), std::stof(teufe)));
					new_object.vertexnumber = new_object.vertexnumber + 1;
				}

				if (key == "PVRTX")
				{
					std::string rw, hw, teufe, dummy3;
					stringstream2 >> rw >> std::ws >> hw >> std::ws >> teufe >> std::ws >> dummy3 >> std::ws;
					new_object.vertices.push_back(Vertex(new_object.vertexnumber, std::stof(rw), std::stof(hw), std::stof(teufe)));
					new_object.vertexnumber = new_object.vertexnumber + 1;
				}

				if (key == "TRGL")
				{
					int32_t a, b;
					stringstream2 >> a >> std::ws >> b >> std::ws;
					a--; b--;
					new_object.triangles.push_back(Triangle(new_object.trianglenumber, std::stoi(rest) - 1, a, b));
					new_object.trianglenumber = new_object.trianglenumber + 1;
				}
			}
			objects.push_back(new_object);
		}
		geotype lastgeo = UNDEF;
		if (key == "GOCAD" && rest == "PLine")
		{
			bool end = false;
			while (!ifs.eof())
			{
				GObj new_object;
				new_object.type = PLINE;
				new_object.vertexnumber = 0;
				new_object.trianglenumber = 0;
				new_object.color = Color(0, 0, 0);
				new_object.geo = lastgeo;

				bool changed = false;

				// first process the header
				while (key != std::string("ILINE"))
				{
					std::getline(ifs, line);
					std::stringstream stringstream2(line);
					key = "";
					rest = "";
					stringstream2 >> key >> std::ws >> rest >> std::ws;

					if (key == "name:")
					{
						new_object.name = rest;
						if (rest.find("am1") != std::string::npos) new_object.geo = am1UK;
						if (rest.find("Sy") != std::string::npos && rest.find("OK") != std::string::npos) new_object.geo = SyOK;
						if (rest.find("Sy") != std::string::npos && rest.find("UK") != std::string::npos) new_object.geo = SyUK;
						if (rest.find("Ref") != std::string::npos || rest.find("ref") != std::string::npos || rest.find("Rad") != std::string::npos || rest.find("rad") != std::string::npos) new_object.geo = REF; // Reflektor
						lastgeo = new_object.geo;
					}
					if (key == "END") { end = true; break; }
				}
				while (!key.empty())
				{
					if (key == "END") { break; }
					std::getline(ifs, line);
					std::stringstream stringstream2(line);
					key = "";
					rest = "";
					stringstream2 >> key >> std::ws >> rest >> std::ws;
					if (key == "VRTX")
					{
						std::string rw, hw, teufe;
						stringstream2 >> rw >> std::ws >> hw >> std::ws >> teufe >> std::ws;
						float nteufe = std::stof(teufe);  if (nteufe > 0) nteufe = nteufe*-1;
						new_object.vertices.push_back(Vertex(std::stof(rw), std::stof(hw), nteufe));
					}

					if (key == "PVRTX")
					{
						std::string rw, hw, teufe, dummy4;
						stringstream2 >> rw >> std::ws >> hw >> std::ws >> teufe >> std::ws >> dummy4 >> std::ws;
						float nteufe = std::stof(teufe);  if (nteufe > 0) nteufe = nteufe*-1;
						new_object.vertices.push_back(Vertex(std::stof(rw), std::stof(hw), nteufe));
					}
					if (key == "SEG")
					{
						break;
					}
				}




				if (!end) objects.push_back(new_object);
				if (key == "END") break;
				// segments not needed - order of vertices defines segments
			}
			
		}

		if (key == "GOCAD" && rest == "Well")
		{
			GObj new_object;
			new_object.type = PLINE;
			new_object.vertexnumber = 0;
			new_object.trianglenumber = 0;
			new_object.color = Color(255, 0, 0); // red
			new_object.geo = WELL;
			// jetzt bis Ende Surface durchgehen
			float refX = 0, refY = 0, refZ = 0;
			while (!ifs.eof() /*&& std::getline(ifs, line)*/)
			{
				std::getline(ifs, line);
				std::stringstream stringstream2(line);
				// n�chste zeilen durchgehen
				key = "";
				rest = "";
				stringstream2 >> key >> std::ws >> rest >> std::ws;
				if (key == "END") break; // Loop der inneren Schleife abbrechen, n�chstes Objekt f�ngt an
				if (key == "name:")
				{
					new_object.name = rest;
				}

				if (key == "WREF") // Bohransatzpunkt
				{
					std::string rw, hw, teufe;
					stringstream2 >> rw >> std::ws >> hw >> std::ws >> teufe >> std::ws;
					new_object.vertices.push_back(Vertex(new_object.vertexnumber, std::stof(rest), std::stof(rw), std::stof(hw)));
					refX = std::stof(rest);
					refY = std::stof(rw);
					refZ = std::stof(hw);
					new_object.vertexnumber = new_object.vertexnumber + 1;
				}

				if (key == "PATH") // Bhrmessung
				{
					std::string rw, hw, teufe;
					stringstream2 >> rw >> std::ws >> hw >> std::ws >> teufe >> std::ws;
					new_object.vertices.push_back(Vertex(new_object.vertexnumber, refX + std::stof(hw), refY + std::stof(teufe), std::stof(rw)));
					new_object.vertexnumber = new_object.vertexnumber + 1;
				}

			}
			objects.push_back(new_object);
		}

		linecounter++;
		if (linecounter == 100000) { std::cout << "Processed 100.000 lines\n"; linecounter = 0; }




	}
	if (objects.size() == 0) { fprintf_s(stderr, "\n\nError: Not a valid GOCAD ASCII file!\n\n"); system("PAUSE"); exit(0); }
	else
	{
		fprintf_s(stderr, "  ...done. \n\n");
		std::cout << "Number of GOCAD objects found:" << objects.size() << "\n";
	}

	// jetzt der zweite teil - alles in html packen
	// =============================================

	std::ofstream html;
	std::string delimiter = ".";
	std::string file_firstname = filename.substr(0, filename.find(delimiter)); // dateiname ohne endung
	html.open(file_firstname + ".html");
	std::string header(reinterpret_cast<const char*>(HTMLheader), sizeof(HTMLheader));
	html << header;



	char* optional = R"=====(
text2.innerHTML = "<img height='42' width='42' src='https://pbs.twimg.com/profile_images/366226936/twitter-logo-kpluss_400x400.jpg'></img>\
<br>\
<b><u>Legende</u></b>\
<br>\
Anhydritmittel (<i>am1</i>)&nbsp;&nbsp;<div style='width:10px;height:5px;border:1px; background-color:green; display: inline-block;'></div><br>\
Fl&ouml;z Ronnenberg (<i>K3RoSy</i>) OK&nbsp;&nbsp;<div style='width:10px;height:5px;border:1px; background-color:red; display: inline-block;'></div><br>\
Fl&ouml;z Ronnenberg (<i>K3RoSy</i>) UK&nbsp;&nbsp;<div style='width:10px;height:5px;border:1px; background-color:yellow; display: inline-block;'></div><br>\
<br>";
)=====";
	html << optional;


	// todo - add text for each TSurf a new line
	// same for wells, for reflectors


	

	char* neu = R"=====(
text2.style.top = 50 + 'px';
text2.style.left = 50 + 'px';
document.body.appendChild(text2);
</script>
<script>
var scene, camera, material, light, ambientLight, renderer;
var meshes = [];
)=====";
	html << neu;
	
	std::string UNDEFline, UNDEFsurfvert, UNDEFsurftri;
	bool UNDEFsurffound = false, UNDEFlinefound = false, DEFfound=false;
	int32_t vertcounter = 0, tricounter = 0, undefcounter = 0;
	int32_t end_i = -1;

	for (int32_t i = 0; i < objects.size(); i++)
	{
		if (objects.at(i).geo != UNDEF)
		{
			DEFfound = true;
			std::string data_1_surf_start = "var object" + std::to_string(end_i+1) + " = [ ";
			html << data_1_surf_start;
			std::string data;

			if (objects.at(i).type == SURF) data = std::to_string(objects.at(i).vertexnumber) + ", " + std::to_string(objects.at(i).trianglenumber) + ", "; // erst anzahl vertices, dann anzahl triangles
			if (objects.at(i).type == PLINE) data = ""; // erst anzahl vertices, dann anzahl triangles
			for (auto v : objects.at(i).vertices) data = data + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", ";
			for (auto t : objects.at(i).triangles)data = data + std::to_string(t.v1) + ", " + std::to_string(t.v2) + ", " + std::to_string(t.v3) + ", ";

			data.pop_back();
			data.pop_back(); // letztes Komma + Leerzeichen l�schen
			html << data;
			char* data_1_surf_end = " ];";
			html << data_1_surf_end; // definierte surfaces/lines sofort einspeisen. 
			end_i++;
		}
		if (objects.at(i).geo == UNDEF)
		{
			if (objects.at(i).type == PLINE)
			{
				UNDEFlinefound = true;
				for (auto v : objects.at(i).vertices) UNDEFline = UNDEFline + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", ";
			}
			if (objects.at(i).type == SURF) 
			{
				UNDEFsurffound = true;
				vertcounter += objects.at(i).vertexnumber;
				tricounter += objects.at(i).trianglenumber;
				for (auto v : objects.at(i).vertices) UNDEFsurfvert = UNDEFsurfvert + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ", ";
				for (auto t : objects.at(i).triangles) UNDEFsurftri = UNDEFsurftri + std::to_string(t.v1+undefcounter) + ", " + std::to_string(t.v2+undefcounter) + ", " + std::to_string(t.v3+undefcounter) + ", ";
				undefcounter = vertcounter;
			}
		}
	}

	// undefinierte PLINES
	if (UNDEFlinefound)
	{
		if (!DEFfound) end_i--;
		std::string data_1_surf_start = "var object" + std::to_string(end_i+1) + " = [ ";
		html << data_1_surf_start;
		UNDEFline.pop_back(); //
		UNDEFline.pop_back(); // letztes Komma + Leerzeichen l�schen
		html << UNDEFline;
		char* data_1_surf_end = " ];";
		html << data_1_surf_end;
		if (!DEFfound) end_i++;
	}

	// undefinierte SURFACES
	if (UNDEFsurffound)
	{
		if (!DEFfound) end_i--;
		std::string data_1_surf_start = "var object" + std::to_string(end_i+2) + " = [ ";
		html << data_1_surf_start;
		std::string tmp = std::to_string(vertcounter) + ", " + std::to_string(tricounter) + ", "; //anfang ist vertex- und trianglenummer
		html << tmp;
		UNDEFsurftri.pop_back(); //
		UNDEFsurftri.pop_back(); // letztes Komma + Leerzeichen l�schen
		html << UNDEFsurfvert;
		html << UNDEFsurftri;
		char* data_1_surf_end = " ];";
		html << data_1_surf_end;
		if (!DEFfound) end_i++;
	}


	// ======================= now LAST PART - FOOTER =====================================
	char* part2_start = R"=====(
init();
render();
function loadObjects()
{
var i = 0;
)====="; 
	html << part2_start;


	// ============================= load first DEFINED meshes ====================================
	for (int i = 0; i < end_i;i++)
	{
		char *assign_start;
		if (objects.at(i).type == SURF) { assign_start = R"=====(
			loadTSurf(
			)====="; }

		else {
		assign_start = R"=====(
			loadPLine(
			)=====";
			}
			std::string name = "object" + std::to_string(i);
			std::string assign_end = "); ";
			html << assign_start << name << assign_end;
			std::string color_middle;
			char* color_start = R"=====(
			meshes[i++].material.color = new THREE.Color("rgb()====="; 
			char* color_end = R"=====()");)====="; 
			color_middle = std::to_string(objects.at(i).color.r) + std::string(", ") + std::to_string(objects.at(i).color.g) + std::string(", ") + std::to_string(objects.at(i).color.b);
			if (objects.at(i).geo == UNDEF) color_middle = std::string("100") + std::string(", ") + std::string("0") + std::string(", ") + std::string("0");
			if (objects.at(i).geo == am1UK) color_middle = std::string("0") + std::string(", ") + std::string("255") + std::string(", ") + std::string("123");
			if (objects.at(i).geo == SyUK) color_middle = std::string("255") + std::string(", ") + std::string("255") + std::string(", ") + std::string("0");
			if (objects.at(i).geo == SyOK) color_middle = std::string("255") + std::string(", ") + std::string("124") + std::string(", ") + std::string("80");
			char* transparency = R"=====(
			meshes[i].material.transparent = true;		
			meshes[i].material.opacity = 0.6;
			)=====";
			if (objects.at(i).type == SURF && objects.at(i).geo != SyOK&& objects.at(i).geo != UNDEF) html << transparency << color_start << color_middle << color_end;
			else html << color_start << color_middle << color_end;
	}
	// ===============================================================================================

	if (UNDEFlinefound)
	{
		char *assign_start;
	
			assign_start = R"=====(
			loadPLine(
			)=====";

		std::string name = "object" + std::to_string(end_i);
		std::string assign_end = "); ";
		html << assign_start << name << assign_end;
		std::string color_middle;
		char* color_start = R"=====(
			meshes[i++].material.color = new THREE.Color("rgb()=====";
		char* color_end = R"=====()");)=====";
		color_middle = std::string("0") + std::string(", ") + std::string("0") + std::string(", ") + std::string("0");
		char* transparency = R"=====(
			meshes[i].material.transparent = true;		
			meshes[i].material.opacity = 0.6;
			)=====";
		html << color_start << color_middle << color_end;
	}

	if (UNDEFsurffound)
	{
		char *assign_start;

		assign_start = R"=====(
			loadTSurf(
			)=====";


		std::string name = "object" + std::to_string(end_i + 1);
		std::string assign_end = "); ";
		html << assign_start << name << assign_end;
		std::string color_middle;
		char* color_start = R"=====(
			meshes[i++].material.color = new THREE.Color("rgb()=====";
		char* color_end = R"=====()");)=====";
		color_middle = std::string("0") + std::string(", ") + std::string("0") + std::string(", ") + std::string("0");
		char* transparency = R"=====(
			meshes[i].material.transparent = true;		
			meshes[i].material.opacity = 0.6;
			)=====";
		html << color_start << color_middle << color_end;
	}


	// ==============================last part
	char* final_part = R"=====(
	addMeshes();
	};
	</script><canvas style="width: 1847px; height: 933px;" height="933" width="1847"></canvas>
	</body></html>
	)====="; 
	html << final_part;

	html.close();
	fprintf_s(stderr, "\nDone writing to .html file.\n\n");
	//system("PAUSE");
	return 0;
}


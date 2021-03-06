#include "MD5_MeshReader.h"

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using glm::mat4;

using std::runtime_error;
using std::string;
using std::stringstream;
using std::getline;

using std::cout;
using std::endl;

// Prototypes -- convenience functions.
std::ostream &operator<<(std::ostream &, const MD5_Vertex &);
std::ostream &operator<<(std::ostream &, const MD5_Triangle &);
std::ostream &operator<<(std::ostream &, const MD5_Weight &);

MD5_MeshInfo MD5_MeshReader::parse(const std::string &filename) {
	mMeshFile.open(filename);

	if(!mMeshFile) {				
		throw runtime_error("Could not open the file.");
	} 

	MD5_MeshInfo mesh;

	processVersion();
	processCommandLine();
	processJointsAndMeshCounts();
	processJoints();
	processMeshes();

	mMeshFile.close();
	mesh.meshes = mMeshes;
	mesh.joints = mJoints;
	
	return mesh;
}

void MD5_MeshReader::processVersion() {	
	string line;
	
	getline(mMeshFile, line); 
	stringstream sstream(line);
	string versionStr;
	int versionNum;

	sstream >> versionStr;
	sstream >> versionNum;

	if(versionNum != 10) {
		throw runtime_error("Only processing version 10 of the MD5 spec.");
	}
}

void MD5_MeshReader::processCommandLine() {
	string line;
	getline(mMeshFile, line);

	// Just by-passing this for now
}

void MD5_MeshReader::processJointsAndMeshCounts() {
	string line;
	
	// Eat spaces
	do {
		getline(mMeshFile, line);
	} while(mMeshFile && line == "");

	if(!mMeshFile) {
		throw runtime_error("EOF reached and no joint count found.");
	}

	stringstream tokens(line);
	string numJointsStr;
	int numJoints = -1;

	tokens >> numJointsStr;
	tokens >> numJoints;
	mJoints.resize(numJoints);

	// Get the mesh info
	do {
		getline(mMeshFile, line);
	} while(mMeshFile && line == "");

	if(!mMeshFile) {
		throw runtime_error("EOF reached and no mesh count found.");
	}

	tokens.clear();
	tokens.str(line);
	
	string numMeshesStr;
	int numMeshes = -1;

	tokens >> numMeshesStr;
	tokens >> numMeshes;
}



void MD5_MeshReader::processJoints() {
	string line;

	do {
		getline(mMeshFile, line);
	} while(mMeshFile && line == "");

	stringstream tokens(line);
	string type;

	tokens >> type;
	// Should be "joints"
	if(type != "joints") {
		throw runtime_error("Expected a joints block.");
	}

	int count = 0;
	getline(mMeshFile, line); // Advance to the next line
	while(mMeshFile.good() && line != "}") {
		buildJoint(line, count++);
		getline(mMeshFile, line);		
	}
}

void MD5_MeshReader::processMeshes() {
	string line;

	// The rest of the file should be mesh section.
	while(mMeshFile) {
		do {
			getline(mMeshFile, line);
		} while(mMeshFile && line == "");

		if(!mMeshFile) {
			// We hit the end of the file.
			return;
		}

		stringstream tokens(line);
		string fieldName;

		tokens >> fieldName;
	
		if(fieldName != "mesh") {
			throw runtime_error("Expected a mesh block.");
		}

		processMesh();
		mMeshes.push_back(mCurMesh);
	}
}

void MD5_MeshReader::processMesh() {
	string line;
	string fieldName;
	stringstream tokens;
	string junk;

	mCurMesh.vertices.clear();
	mCurMesh.triangles.clear();
	mCurMesh.weights.clear();

	getline(mMeshFile, line); // Advance to the next line
	tokens.str(line);
	
	tokens >> fieldName;

	if(fieldName != "shader") {
		cout << "No shader in mesh. Do something." << endl;
	}

	string quotedFilename;
	tokens >> quotedFilename;
	mCurMesh.textureFilename = quotedFilename.substr(1, quotedFilename.size() - 2);
	
	// Eat spaces
	do {
		getline(mMeshFile, line);
	} while(mMeshFile && line == "");

	tokens.clear();
	tokens.str(line);
	tokens >> fieldName;
	int numVerts;


	if(fieldName != "numverts") {
		cout << "numverts must be next. Do something." << endl;
		return;
	}

	tokens >> numVerts;
	mCurMesh.vertices.resize(numVerts);

	// Read the verts
	while(mMeshFile) {
		if(line != "") {			
			tokens.clear();
			tokens.str(line);
			tokens >> fieldName;

			if(fieldName == "numtris") {
				break;
			}
			if(fieldName == "vert") {
				MD5_Vertex vertex;
				int index;
				tokens >> index;
				tokens >> junk;
				tokens >> vertex.u;
				tokens >> vertex.v;
				tokens >> junk;
				tokens >> vertex.startWeight;
				tokens >> vertex.weightCount;
				mCurMesh.vertices[index] = vertex;
			}			
		}
		
		getline(mMeshFile, line);
	}

	int numTris;
	tokens >> numTris;
	mCurMesh.triangles.resize(numTris);
	
	// Read the tris
	while(mMeshFile) {
		if(line != "") {
			tokens.clear();
			tokens.str(line);
			tokens >> fieldName;

			if(fieldName == "numweights") {
				break;
			}
			if(fieldName == "tri") {
				int index;
				MD5_Triangle tri;
				tokens >> index;
				tokens >> tri.indices[0];
				tokens >> tri.indices[1];
				tokens >> tri.indices[2];

				mCurMesh.triangles[index] = tri;
			}			
		}

		getline(mMeshFile, line);		
	}
	
	// Read the weights
	int numWeights;
	tokens >> numWeights;
	mCurMesh.weights.resize(numWeights);

	while(mMeshFile) {
		if(line != "") {
			tokens.clear();
			tokens.str(line);
			tokens >> fieldName;
			
			if(fieldName == "}") {
				break;
			}
			if(fieldName == "weight") {
				MD5_Weight weight;
				int index;
				tokens >> index;
				tokens >> weight.jointIndex;
				tokens >> weight.weightBias;
				tokens >> junk;
				tokens >> weight.position.x;
				tokens >> weight.position.y;
				tokens >> weight.position.z;
				tokens >> junk;

				mCurMesh.weights[index] = weight;
			}			
		}
		
		getline(mMeshFile, line);
	}
}

mat4 MD5_MeshReader::getChildToParentMatrix(const Joint &joint) {
	mat4 rotM(glm::mat4_cast(joint.orientation));
	mat4 transM = glm::translate(mat4(1.0f), joint.position);
	
	return transM * rotM;
}

void MD5_MeshReader::computeJointToWorld(Joint &joint) {
	mat4 P(1.0);
	const Joint *pJoint = &joint;

	while(pJoint) {		
		mat4 P_next = getChildToParentMatrix(*pJoint);
		P = P_next * P;

		pJoint = (pJoint->parentIndex == -1) ? nullptr : &mJoints[pJoint->parentIndex];
	}
		 
	joint.jointToWorld = P;
}

void MD5_MeshReader::computeWComponent(Joint &j) {
	// w = +/- sqrt(1 - x^2 - y^2 - z^2)
	// Convention dictates using the negative version
	float x = j.orientation.x;
	float y = j.orientation.y;
	float z = j.orientation.z;
	float w;

	float temp = 1.0f - x*x - y*y - z*z;
	if(temp < 0.0) {
		w = 0.0f;
	} else {
		w = -sqrtf(temp);
	}

	j.orientation.w = w;
}

std::ostream &operator<<(std::ostream &out, const MD5_Vertex &vertex) {
	out << vertex.u << ", " << vertex.v;
	out << " " << vertex.startWeight << " ";
	out << vertex.weightCount;
	return out;
}

std::ostream &operator<<(std::ostream &out, const glm::vec3 &v) {
	out << "<" << v.x << ", " << v.y << ", " << v.z << ">";
	return out;
}

std::ostream &operator<<(std::ostream &out, const MD5_Triangle &tri) {
	out << "<" << tri.indices[0] << ", " << tri.indices[1] << ", " << tri.indices[2] << ">" << endl;
	return out;
}

std::ostream &operator<<(std::ostream &out, const MD5_Weight &weight) {
	cout << "[jointIndex: " << weight.jointIndex << " bias: " << weight.weightBias << " pos: " << weight.position << "]";

	return out;
}

std::ostream & operator<<(std::ostream &out, Joint &joint) {
	out << joint.name << " ";
	out << joint.position.x << ", " << joint.position.y << ", " << joint.position.z;
	out << "[";
	out << joint.orientation.w << " (" 
		<< joint.orientation.x << ", " 
		<< joint.orientation.y << ", "
		<< joint.orientation.z << ")]";

	return out;
}

void MD5_MeshReader::buildJoint(const string &line, int count) {		
	Joint j;
	stringstream tokens(line);
	string dump;
	float orientW, orientX, orientY, orientZ;

	tokens >> j.name;
	tokens >> j.parentIndex;
	tokens >> dump;
	tokens >> j.position.x;
	tokens >> j.position.y;
	tokens >> j.position.z;
	tokens >> dump;
	tokens >> dump;
	tokens >> orientX;
	tokens >> orientY;
	tokens >> orientZ;

	j.orientation.x = orientX;
	j.orientation.y = orientY;
	j.orientation.z = orientZ;
	computeWComponent(j);

	computeJointToWorld(j);

	mJoints[count] = j;	
}
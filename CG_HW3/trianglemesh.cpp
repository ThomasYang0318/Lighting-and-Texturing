#include "trianglemesh.h"
#include <cstddef> // offsetof

// Constructor of a triangle mesh.
TriangleMesh::TriangleMesh()
{
	// -------------------------------------------------------
	// Add your initialization code here.
	vboId        = 0;
    numVertices  = 0;
    numTriangles = 0;
    objCenter    = glm::vec3(0.0f);
    objExtent    = glm::vec3(0.0f);

    vertices.clear();
    subMeshes.clear();
    materials.clear();
	// -------------------------------------------------------
}

// Destructor of a triangle mesh.
TriangleMesh::~TriangleMesh()
{
	// -------------------------------------------------------
	// Add your release code here.
	// delete VBO
    if (vboId != 0) {
        glDeleteBuffers(1, &vboId);
        vboId = 0;
    }

    // delete IBOs
    for (SubMesh& sm : subMeshes) {
        if (sm.iboId != 0) {
            glDeleteBuffers(1, &sm.iboId);
            sm.iboId = 0;
        }
        sm.material = nullptr;
    }
    subMeshes.clear();

    // delete materials
    for (PhongMaterial* m : materials) {
        delete m;
    }
    materials.clear();

    vertices.clear();
	for (auto* t : textures) delete t;
	textures.clear();

	for (auto* m : materials) delete m;
	materials.clear();
	// -------------------------------------------------------
}

static bool LoadMtlFile(
    const std::string& mtlPath,
    std::unordered_map<std::string, PhongMaterial*>& mtlMap,
    std::vector<PhongMaterial*>& mtlList,
    std::vector<ImageTexture*>& texList){
    std::ifstream file(mtlPath);
    if (!file.is_open()) {
        std::cerr << "[TriangleMesh] Failed to open MTL file: " << mtlPath << std::endl;
        return false;
    }

    std::string line;
    PhongMaterial* current = nullptr;

    std::string baseDir = "./";
    size_t slashPos = mtlPath.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        baseDir = mtlPath.substr(0, slashPos + 1);
    }

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "newmtl") {
            std::string name;
            iss >> name;
            current = new PhongMaterial();
            current->SetName(name);
            mtlMap[name] = current;
            mtlList.push_back(current);
        }
        else if (tag == "Ka" && current) {
            float r, g, b; iss >> r >> g >> b;
            current->SetKa(glm::vec3(r, g, b));
        }
        else if (tag == "Kd" && current) {
            float r, g, b; iss >> r >> g >> b;
            current->SetKd(glm::vec3(r, g, b));
        }
        else if (tag == "Ks" && current) {
            float r, g, b; iss >> r >> g >> b;
            current->SetKs(glm::vec3(r, g, b));
        }
        else if (tag == "Ns" && current) {
            float n; iss >> n;
            current->SetNs(n);
        }
        else if (tag == "map_Kd" && current) {
            std::string texName;
            iss >> texName;
            if (!texName.empty()) {
                std::string texPath = baseDir + texName;
                ImageTexture* tex = new ImageTexture(texPath);
                if (tex->IsValid()) {
                    current->SetMapKd(tex);
                    texList.push_back(tex);
                } else {
                    delete tex;
                }
            }
        }
    }

    file.close();
    return true;
}

// Load the geometry and material data from an OBJ file.
bool TriangleMesh::LoadFromFile(const std::string& filePath, const bool normalized)
{	
	// Parse the OBJ file.
	// ---------------------------------------------------------------------------
    // Add your implementation here (HW1 + read *.MTL).
	// Clean up any existing data.
    if (vboId != 0) {
        glDeleteBuffers(1, &vboId);
        vboId = 0;
    }
    for (SubMesh& sm : subMeshes) {
        if (sm.iboId != 0) {
            glDeleteBuffers(1, &sm.iboId);
            sm.iboId = 0;
        }
        sm.material = nullptr;
    }
    subMeshes.clear();
    for (PhongMaterial* m : materials) {
        delete m;
    }
    materials.clear();
    vertices.clear();

    numVertices  = 0;
    numTriangles = 0;
    objCenter    = glm::vec3(0.0f);
    objExtent    = glm::vec3(0.0f);

    // open the obj file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "[TriangleMesh] Failed to open file: " << filePath << std::endl;
        return false;
    }

    // Get base directory for .mtl files
    std::string baseDir = "./";
    size_t slashPos = filePath.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        baseDir = filePath.substr(0, slashPos + 1);
    }

    std::vector<glm::vec3> posPool;
    std::vector<glm::vec2> texPool;
    std::vector<glm::vec3> nrmPool;

    std::string line;

    // v/vt/vn → shared vertex index
    std::unordered_map<std::string, unsigned> remap;

    // material name → PhongMaterial*
    std::unordered_map<std::string, PhongMaterial*> mtlMap;

    // submesh id
    int currentSubMeshId = -1;
    std::unordered_map<std::string, int> subMeshMap; // material name → submesh index

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string tag;
        iss >> tag;

        if (tag == "v") {
            float x, y, z;
            iss >> x >> y >> z;
            posPool.push_back(glm::vec3(x, y, z));
        }
        else if (tag == "vt") {
            float u, v;
            iss >> u >> v;
            texPool.push_back(glm::vec2(u, v));
        }
        else if (tag == "vn") {
            float x, y, z;
            iss >> x >> y >> z;
            nrmPool.push_back(glm::vec3(x, y, z));
        }
        else if (tag == "mtllib") {
            // load the .mtl file
            std::string mtlFileName;
            iss >> mtlFileName;
            std::string mtlPath = baseDir + mtlFileName;
            LoadMtlFile(mtlPath, mtlMap, materials, textures);
        }
        else if (tag == "usemtl") {
            std::string mtlName;
            iss >> mtlName;

            // Find the corresponding submesh for this material, create a new one if it doesn't exist
            auto itSM = subMeshMap.find(mtlName);
            if (itSM != subMeshMap.end()) {
                currentSubMeshId = itSM->second;
            } else {
                SubMesh sm;
                sm.iboId    = 0;

                // assign material
                PhongMaterial* mtlPtr = nullptr;
                auto itM = mtlMap.find(mtlName);
                if (itM != mtlMap.end()) {
                    mtlPtr = itM->second;
                }
                sm.material = mtlPtr;

                subMeshes.push_back(sm);
                int newId = (int)subMeshes.size() - 1;
                subMeshMap[mtlName] = newId;
                currentSubMeshId = newId;
            }
        }
        else if (tag == "f") {
            // ensure we have a submesh to add faces to
            if (currentSubMeshId < 0) {
                SubMesh sm;
                sm.iboId    = 0;
                sm.material = nullptr; // no material assigned yet
                subMeshes.push_back(sm);
                currentSubMeshId = 0;
            }

            std::vector<unsigned> faceIdx;
            std::string token;

            while (iss >> token) {
                auto it = remap.find(token);
                unsigned idx;

                if (it != remap.end()) {
                    idx = it->second;
                } else {
                    int v = 0, vt = 0, vn = 0;
                    if (sscanf(token.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {
                        // v/vt/vn
                    }
                    else if (sscanf(token.c_str(), "%d//%d", &v, &vn) == 2) {
                        vt = 0;
                    }
                    else if (sscanf(token.c_str(), "%d/%d", &v, &vt) == 2) {
                        vn = 0;
                    }
                    else {
                        sscanf(token.c_str(), "%d", &v);
                        vt = 0;
                        vn = 0;
                    }

                    v  = v  - 1;
                    vt = vt - 1;
                    vn = vn - 1;

                    VertexPTN vert;

                    if (v  >= 0 && v  < (int)posPool.size())
                        vert.position = posPool[v];
                    if (vt >= 0 && vt < (int)texPool.size())
                        vert.texcoord = texPool[vt];
                    if (vn >= 0 && vn < (int)nrmPool.size())
                        vert.normal   = nrmPool[vn];

                    vertices.push_back(vert);
                    idx = (unsigned)vertices.size() - 1;
                    remap[token] = idx;
                }

                faceIdx.push_back(idx);
            }

            // triangulate the face (fan triangulation)
            SubMesh& sm = subMeshes[currentSubMeshId];
            for (size_t i = 1; i + 1 < faceIdx.size(); ++i) {
                sm.vertexIndices.push_back(faceIdx[0]);
                sm.vertexIndices.push_back(faceIdx[i]);
                sm.vertexIndices.push_back(faceIdx[i + 1]);
            }
        }
    }

    file.close();

    if (vertices.empty() || subMeshes.empty()) {
        std::cerr << "[TriangleMesh] No valid geometry loaded from: "
                  << filePath << std::endl;
        return false;
    }

    numVertices = (int)vertices.size();
    numTriangles = 0;
    for (const SubMesh& sm : subMeshes) {
        numTriangles += (int)sm.vertexIndices.size() / 3;
    }
    // ---------------------------------------------------------------------------

	// Normalize the geometry data.
	glm::vec3 minP(FLT_MAX);
    glm::vec3 maxP(-FLT_MAX);

    for (const VertexPTN& v : vertices) {
        minP.x = std::min(minP.x, v.position.x);
        minP.y = std::min(minP.y, v.position.y);
        minP.z = std::min(minP.z, v.position.z);

        maxP.x = std::max(maxP.x, v.position.x);
        maxP.y = std::max(maxP.y, v.position.y);
        maxP.z = std::max(maxP.z, v.position.z);
    }

    objCenter = 0.5f * (minP + maxP);
    objExtent = maxP - minP;
	if (normalized) {
		// -----------------------------------------------------------------------
		// Add your normalization code here (HW1).
		float maxExtent = std::max(objExtent.x, std::max(objExtent.y, objExtent.z));
        if (maxExtent > 0.0f) {
            float scale = 1.0f / maxExtent;

            for (VertexPTN& v : vertices) {
                v.position = (v.position - objCenter) * scale;
            }

            minP = vertices[0].position;
            maxP = vertices[0].position;
            for (const VertexPTN& v : vertices) {
                minP.x = std::min(minP.x, v.position.x);
                minP.y = std::min(minP.y, v.position.y);
                minP.z = std::min(minP.z, v.position.z);

                maxP.x = std::max(maxP.x, v.position.x);
                maxP.y = std::max(maxP.y, v.position.y);
                maxP.z = std::max(maxP.z, v.position.z);
            }
            objCenter = 0.5f * (minP + maxP);
            objExtent = maxP - minP;
        }
		// -----------------------------------------------------------------------
	}
	return true;
}

void TriangleMesh::CreateGLBuffers()
{
	if (vboId == 0 && !vertices.empty()) {
		glGenBuffers(1, &vboId);
		glBindBuffer(GL_ARRAY_BUFFER, vboId);
		glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPTN) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	}

	for (SubMesh& sm : subMeshes) {
		if (sm.iboId == 0 && !sm.vertexIndices.empty()) {
			glGenBuffers(1, &sm.iboId);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.iboId);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(unsigned int) * sm.vertexIndices.size(),
				&sm.vertexIndices[0],
				GL_STATIC_DRAW);
		}
	}
}

void TriangleMesh::Render(PhongShadingDemoShaderProg* shader, const glm::mat4& MVP)
{
	if (shader == nullptr || vertices.empty() || subMeshes.empty())
		return;

	if (vboId == 0)
		CreateGLBuffers();

	GLint prog = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prog);

	GLint locPos = glGetAttribLocation(prog, "Position");
	GLint locNrm = glGetAttribLocation(prog, "Normal");
	GLint locUV  = glGetAttribLocation(prog, "Texcoord");

	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	if (locPos >= 0) {
		glEnableVertexAttribArray(locPos);
		glVertexAttribPointer(
			locPos, 3, GL_FLOAT, GL_FALSE,
			sizeof(VertexPTN),
			(void*)offsetof(VertexPTN, position));
	}
	if (locNrm >= 0) {
		glEnableVertexAttribArray(locNrm);
		glVertexAttribPointer(
			locNrm, 3, GL_FLOAT, GL_FALSE,
			sizeof(VertexPTN),
			(void*)offsetof(VertexPTN, normal));
	}
	if (locUV >= 0) {
		glEnableVertexAttribArray(locUV);
		glVertexAttribPointer(
			locUV, 2, GL_FLOAT, GL_FALSE,
			sizeof(VertexPTN),
			(void*)offsetof(VertexPTN, texcoord));
	}

	for (const SubMesh& sm : subMeshes) {
		if (sm.iboId == 0 || sm.vertexIndices.empty())
			continue;

		if (shader->GetLocHasMapKd() >= 0) {
			glUniform1i(shader->GetLocHasMapKd(), 0);
		}
		if (sm.material != nullptr) {
			glUniform3fv(shader->GetLocKa(), 1, glm::value_ptr(sm.material->GetKa()));
			glUniform3fv(shader->GetLocKd(), 1, glm::value_ptr(sm.material->GetKd()));
			glUniform3fv(shader->GetLocKs(), 1, glm::value_ptr(sm.material->GetKs()));
			glUniform1f(shader->GetLocNs(), sm.material->GetNs());

			if (sm.material->GetMapKd() != nullptr && sm.material->GetMapKd()->IsValid()) {
				sm.material->GetMapKd()->Bind(GL_TEXTURE0);
				glUniform1i(shader->GetLocMapKd(), 0);
				if (shader->GetLocHasMapKd() >= 0) {
					glUniform1i(shader->GetLocHasMapKd(), 1);
				}
			} else if (shader->GetLocHasMapKd() >= 0) {
				glUniform1i(shader->GetLocHasMapKd(), 0);
			}
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sm.iboId);
		glDrawElements(GL_TRIANGLES, (GLsizei)sm.vertexIndices.size(), GL_UNSIGNED_INT, 0);
	}

	if (locPos >= 0) glDisableVertexAttribArray(locPos);
	if (locNrm >= 0) glDisableVertexAttribArray(locNrm);
	if (locUV >= 0) glDisableVertexAttribArray(locUV);
}

// Show model information.
void TriangleMesh::ShowInfo()
{
	std::cout << "# Vertices: " << numVertices << std::endl;
	std::cout << "# Triangles: " << numTriangles << std::endl;
	std::cout << "Total " << subMeshes.size() << " subMeshes loaded" << std::endl;
	for (unsigned int i = 0; i < subMeshes.size(); ++i) {
		const SubMesh& g = subMeshes[i];
		std::cout << "SubMesh " << i << " with material: " << g.material->GetName() << std::endl;
		std::cout << "Num. triangles in the subMesh: " << g.vertexIndices.size() / 3 << std::endl;
	}
	std::cout << "Model Center: " << objCenter.x << ", " << objCenter.y << ", " << objCenter.z << std::endl;
	std::cout << "Model Extent: " << objExtent.x << " x " << objExtent.y << " x " << objExtent.z << std::endl;
}


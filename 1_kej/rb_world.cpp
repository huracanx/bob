#include "pch.h"


void r::RBW_TerrainClips()
{
	if (!v::mod_wcol_patch_clips.isEnabled())
		return;

	static std::vector<v3>& v = cworld.terrain.clip_points;
	vec3_t tris[3];
	BYTE col[4];
	vec3_t out;


	if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
		std::cout << "v.size(): " << v.size() << '\n';
	}

	if (v.size() >= 3) {
		for (int i = 0; i < v.size() - 3; i += 3) {



			tris[0][0] = v[i].x;
			tris[0][1] = v[i].y;
			tris[0][2] = v[i].z;

			tris[1][0] = v[i + 1].x;
			tris[1][1] = v[i + 1].y;
			tris[1][2] = v[i + 1].z;

			tris[2][0] = v[i + 2].x;
			tris[2][1] = v[i + 2].y;
			tris[2][2] = v[i + 2].z;

			VectorSubtract(tris[0], clients->cgameOrigin, out);
			const float mag = sqrt(out[0] * out[0] + out[1] * out[1] + out[2] * out[2]);

			if (mag > v::mod_wcol_drawdist.GetFloat())
				continue;

			R_ConvertColorToBytes(vec4_t{ 1,0,0,v::mod_wcol_alpha.GetFloat() }, col);

			RB_DrawPolyInteriors(3, tris, col, v::mod_wcol_two_sided.isEnabled(), v::mod_wcol_depth.isEnabled());
			//RB_DrawTriangleOutline(tris, vec4_t{ 255,0,0,255 }, 1, v::mod_wcol_depth.isEnabled());
		}
	}
}

namespace {
    struct SlopeTriangle { glm::vec3 points[3]; glm::vec3 center; float normalZ; };
    std::vector<SlopeTriangle> slopeTriangles;
    bool slopeCacheValid = false;
    std::string slopeMap;
    unsigned int slopeChecksum = 0;
    const void* slopeVerts = nullptr;
    const void* slopeBrushes = nullptr;

    bool IsBounceNormal(float z) { return std::isfinite(z) && z >= .3f && z <= .7f; }
    glm::vec3 AsVec(const float* p) { return glm::vec3(p[0], p[1], p[2]); }
    void AddSlope(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float z) {
        if (!IsBounceNormal(z) || slopeTriangles.size() >= 200000u) return;
        slopeTriangles.push_back({{a,b,c}, (a+b+c)/3.f, z});
    }
    // Intersect the sloped brush plane with all other half-spaces (including
    // axial bounds). This avoids calling a version-specific native winding API.
    void ClipFace(std::vector<glm::vec3>& face, const glm::vec3& n, float dist) {
        if (face.empty()) return;
        std::vector<glm::vec3> out;
        for (size_t i=0; i<face.size(); ++i) {
            const auto& a=face[i]; const auto& b=face[(i+1)%face.size()];
            const float da=glm::dot(n,a)-dist, db=glm::dot(n,b)-dist;
            const bool inA=da<=.02f, inB=db<=.02f;
            if (inA) out.push_back(a);
            if (inA!=inB && std::fabs(da-db)>1e-6f)
                out.push_back(a+(b-a)*glm::clamp(da/(da-db),0.f,1.f));
        }
        face.swap(out);
    }
    void BuildSlopeCache() {
        slopeTriangles.clear();
        if (cm->verts && cm->triIndices && cm->triCount > 0) {
            for (int i=0;i<cm->triCount;++i) {
                const size_t j=static_cast<size_t>(i)*3;
                const unsigned int a=cm->triIndices[j], b=cm->triIndices[j+1], c=cm->triIndices[j+2];
                if (a>=cm->vertCount || b>=cm->vertCount || c>=cm->vertCount) continue;
                vec4_t plane{};
                if (PlaneFromPoints(plane,cm->verts[a],cm->verts[b],cm->verts[c]))
                    AddSlope(AsVec(cm->verts[a]),AsVec(cm->verts[b]),AsVec(cm->verts[c]),plane[2]);
            }
        }
        if (cm->brushes) for (unsigned int i=0;i<cm->numBrushes;++i) {
            const auto& brush=cm->brushes[i];
            if (brush.isSubmodel || !(brush.contents & MASK_PLAYERSOLID) || !brush.sides || brush.numsides>128) continue;
            const glm::vec3 lo=AsVec(brush.mins), hi=AsVec(brush.maxs), center=(lo+hi)*.5f;
            const float radius=glm::length(hi-lo)+1.f;
            if (!std::isfinite(radius) || radius<=1.f) continue;
            for (unsigned int j=0;j<brush.numsides;++j) {
                const auto* plane=brush.sides[j].plane;
                if (!plane || !IsBounceNormal(plane->normal[2])) continue;
                const glm::vec3 n=AsVec(plane->normal);
                const glm::vec3 u=glm::normalize(glm::cross(n,glm::vec3(0,0,1)))*radius;
                const glm::vec3 v=glm::normalize(glm::cross(n,u))*radius;
                const glm::vec3 c=center+n*(plane->dist-glm::dot(n,center));
                std::vector<glm::vec3> face={c-u-v,c+u-v,c+u+v,c-u+v};
                for (int axis=0;axis<3;++axis) {
                    glm::vec3 unit(0.f); unit[axis]=1.f;
                    ClipFace(face,unit,hi[axis]); ClipFace(face,-unit,-lo[axis]);
                }
                for (unsigned int k=0;k<brush.numsides && !face.empty();++k) {
                    if (k==j || !brush.sides[k].plane) continue;
                    const auto* other=brush.sides[k].plane;
                    ClipFace(face,AsVec(other->normal),other->dist);
                }
                for (size_t k=2;k<face.size();++k) AddSlope(face[0],face[k-1],face[k],plane->normal[2]);
            }
        }
    }
}
void r::RBW_BounceableSlopes() {
    if (!v::mod_bounceable_surf.isEnabled() || !VID_ACTIVE || !cm->isInUse || !cm->name) {
        slopeCacheValid=false; slopeTriangles.clear(); return;
    }
    if (!slopeCacheValid || slopeMap!=cm->name || slopeChecksum!=cm->checksum ||
        slopeVerts!=cm->verts || slopeBrushes!=cm->brushes) {
        slopeMap=cm->name; slopeChecksum=cm->checksum; slopeVerts=cm->verts; slopeBrushes=cm->brushes;
        BuildSlopeCache(); slopeCacheValid=true;
    }
    const glm::vec3 eye=AsVec(clients->cgameOrigin);
    const float range=2500.f;
    unsigned int drawn=0;
    for (const auto& triangle:slopeTriangles) {
        const glm::vec3 diff=triangle.center-eye;
        if (glm::dot(diff,diff)>range*range) continue;
        vec3_t points[3];
        for (int i=0;i<3;++i) for (int j=0;j<3;++j) points[i][j]=triangle.points[i][j];
        const float t=glm::clamp((triangle.normalZ-.3f)/.4f,0.f,1.f);
        BYTE color[4]; R_ConvertColorToBytes(vec4_t{.25f+.7f*t,1.f-.55f*t,.25f,.36f},color);
        RB_DrawPolyInteriors(3,points,color,true,true);
        if (++drawn>=512) break;
    }
}

#ifndef _SPINE_NODE_H_
#define _SPINE_NODE_H_
#include <string>
#include <map>
#include "texture_loader.h"
#include "graphic_engine/drawable/texture.h"
#include "graphic_engine/node2d.h"
#include "spine/Skeleton.h"
#include "spine/AnimationState.h"
#include "spine/AtlasAttachmentLoader.h"
#include "spine/SkeletonClipping.h"

using namespace cubicat;

class SpineNode : public Node2D
{
public:
    DECLARE_RTTI_SUB(SpineNode, Node2D)
    SharedPtr<SpineNode> static create() {return SharedPtr<SpineNode>(NEW SpineNode());}
    ~SpineNode();

    void update(float deltaTime, bool parentDirty) override;
    // [JS_BINDING_BEGIN]
    static SpineNode* createSpine();
    void loadWithBinaryFile(const std::string &skeletonBinaryFile, const std::string &atlasFile, float scale = 1.0f);
    void unload();
    void setSkinByName(const std::string &skinName);
    void setSkinByIndex(int idx);
    TrackEntry* setAnimation(int trackIndex, const std::string &name, bool loop);
    TrackEntry* addAnimation(int trackIndex, const std::string &name, bool loop, float delay = 0.0f);
    void clearTrack(int trackIndex);
    // [JS_BINDING_END]
    TrackEntry* setAnimation(int trackIndex, int animIndex, bool loop);
    TrackEntry* addAnimation(int trackIndex, int animIndex, bool loop, float delay = 0.0f);
    // [JS_BINDING_BEGIN]
    void setScale(const Vector2f& scale);
    void setPosition(const Vector2f& pos);
    void useBilinearFilter(bool b);
    // [JS_BINDING_END]
    
    const std::vector<std::string>& getAnimationNames();
private:
    SpineNode();
    void updateMesh();
    void initialize();
    std::string getAnimationName(int idx);
    static CubicatTextureLoader         m_sTextureLoader;
    Skeleton*                           m_pSkeleton = nullptr;
    AnimationState*                     m_pAnimState = nullptr;
    SkeletonClipping*                   m_pClipper = nullptr;
    AtlasAttachmentLoader*              m_pAttachmentLoader = nullptr;
    Atlas*                              m_pAtlas = nullptr;
    std::map<std::string,TexturePtr>    m_textureMap;
    std::vector<std::string>            m_vAnimationNames;
    bool                                m_bUseBilinearFilter = false;
};
typedef SharedPtr<SpineNode> SpineAnimationPtr;

#endif
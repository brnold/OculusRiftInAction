#include "Common.h"

struct PerEyeArg {
  FramebufferWrapperPtr  framebuffer;
  glm::mat4 projection;
};

class CubeScene_RiftTimewarp : public RiftGlfwApp {
  PerEyeArg eyes[2];
  ovrTexture eyeTextures[2];
  ovrVector3f eyeOffsets[2];

  float ipd{ OVR_DEFAULT_IPD };
  float eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };

public:
  CubeScene_RiftTimewarp() {
    eyeHeight = ovrHmd_GetFloat(hmd, OVR_KEY_EYE_HEIGHT, eyeHeight);
    ipd = ovrHmd_GetFloat(hmd, OVR_KEY_IPD, ipd);

    Stacks::modelview().top() = glm::lookAt(
      vec3(0, eyeHeight, 5 * ipd),
      vec3(0, eyeHeight, 0),
      Vectors::UP);

    if (!ovrHmd_ConfigureTracking(hmd,
      ovrTrackingCap_Orientation |
      ovrTrackingCap_Position, 0)) {
      SAY("Warning: Unable to locate Rift sensor device.  This demo is boring now.");
    }
  }

  virtual void initGl() {
    GlfwApp::initGl();

    ovrRenderAPIConfig cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.Header.API = ovrRenderAPI_OpenGL;
    cfg.Header.BackBufferSize = ovr::fromGlm(getSize());
    cfg.Header.Multisample = 1;

    int distortionCaps =
      ovrDistortionCap_TimeWarp |
      ovrDistortionCap_Chromatic |
      ovrDistortionCap_Vignette;
    ovrEyeRenderDesc eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg,
      distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye) {
      PerEyeArg & eyeArgs = eyes[eye];
      ovrFovPort fov = hmd->DefaultEyeFov[eye];
      ovrSizei texSize = ovrHmd_GetFovTextureSize(hmd, eye, fov, 1.0f);
      eyeArgs.framebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
      eyeArgs.framebuffer->init(ovr::toGlm(texSize));

      ovrTextureHeader & textureHeader = eyeTextures[eye].Header;
      textureHeader.API = ovrRenderAPI_OpenGL;
      textureHeader.TextureSize = texSize;
      textureHeader.RenderViewport.Size = texSize;
      textureHeader.RenderViewport.Pos.x = 0;
      textureHeader.RenderViewport.Pos.y = 0;
      ((ovrGLTextureData&)eyeTextures[eye]).TexId =
        oglplus::GetName(eyeArgs.framebuffer->color);

      eyeOffsets[eye] =
        eyeRenderDescs[eye].HmdToEyeViewOffset;

      ovrMatrix4f projection = ovrMatrix4f_Projection(fov, 0.01f, 100, true);
      eyeArgs.projection = ovr::toGlm(projection);
    });
  }


  void onKey(int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
      static ovrHSWDisplayState hswDisplayState;
      ovrHmd_GetHSWDisplayState(hmd, &hswDisplayState);
      if (hswDisplayState.Displayed) {
        ovrHmd_DismissHSWDisplay(hmd);
        return;
      }
    }

    if (GLFW_PRESS != action) {
      GlfwApp::onKey(key, scancode, action, mods);
      return;
    }

    switch (key) {
    case GLFW_KEY_R:
      ovrHmd_RecenterPose(hmd);
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }


  virtual void finishFrame() {
  }

  virtual void draw() {
    ovrPosef eyePoses[2];
    ovrHmd_GetEyePoses(hmd, getFrame(), eyeOffsets, eyePoses, nullptr);

    ovrHmd_BeginFrame(hmd, getFrame());
    MatrixStack & mv = Stacks::modelview();
    for (int i = 0; i < ovrEye_Count; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      PerEyeArg & eyeArgs = eyes[eye];
      Stacks::projection().top() = eyeArgs.projection;

      eyeArgs.framebuffer->Bind();
      oglplus::Context::Clear().DepthBuffer();
      Stacks::withPush(mv, [&] {
        mv.preMultiply(glm::inverse(ovr::toGlm(eyePoses[eye])));
        oria::renderExampleScene(ipd, eyeHeight);
      });
    }
    ovrHmd_EndFrame(hmd, eyePoses, eyeTextures);
  }
};

RUN_OVR_APP(CubeScene_RiftTimewarp);


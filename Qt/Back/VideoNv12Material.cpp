#include "VideoNv12Material.hpp"
#include <QMatrix4x4>
#include <QtCore/qglobal.h>
#include <cstring>

namespace {
struct ShaderResourceInit {
  ShaderResourceInit() { Q_INIT_RESOURCE(BackEndShaders); }
};
static ShaderResourceInit g_shaderResourceInit;
} // namespace

QSGMaterialType VideoNv12Material::type_;

namespace {
class VideoNv12Shader : public QSGMaterialShader {
public:
  VideoNv12Shader() {
    setShaderFileName(VertexStage,
                      QStringLiteral(":/back/shaders/nv12.vert.qsb"));
    setShaderFileName(FragmentStage,
                      QStringLiteral(":/back/shaders/nv12.frag.qsb"));
  }

  bool updateUniformData(RenderState &state, QSGMaterial *newMaterial,
                         QSGMaterial *) override {
    Q_UNUSED(newMaterial);
    QByteArray *buf = state.uniformData();
    if (!buf)
      return false;

    if (buf->size() < 80)
      buf->resize(80);

    const QMatrix4x4 m = state.combinedMatrix();
    std::memcpy(buf->data(), m.constData(), 64);

    const float opacity = state.opacity();
    std::memcpy(buf->data() + 64, &opacity, 4);

    auto *mat = static_cast<VideoNv12Material *>(newMaterial);
    const float swapUV = mat->swapUV() ? 1.0f : 0.0f;
    const float use709 = mat->use709() ? 1.0f : 0.0f;
    const float fullRange = mat->fullRange() ? 1.0f : 0.0f;
    std::memcpy(buf->data() + 68, &swapUV, 4);
    std::memcpy(buf->data() + 72, &use709, 4);
    std::memcpy(buf->data() + 76, &fullRange, 4);

    return true;
  }

  void updateSampledImage(RenderState &, int binding, QSGTexture **texture,
                          QSGMaterial *newMaterial,
                          QSGMaterial *) override {
    auto *mat = static_cast<VideoNv12Material *>(newMaterial);
    if (binding == 1) {
      *texture = mat->textureY();
    } else if (binding == 2) {
      *texture = mat->textureUV();
    }
  }
};
} // namespace

VideoNv12Material::VideoNv12Material() { setFlag(Blending, false); }

void VideoNv12Material::setTextures(QSGTexture *texY, QSGTexture *texUV) {
  texY_ = texY;
  texUV_ = texUV;
}

QSGMaterialType *VideoNv12Material::type() const { return &type_; }

QSGMaterialShader *
VideoNv12Material::createShader(QSGRendererInterface::RenderMode) const {
  return new VideoNv12Shader();
}

int VideoNv12Material::compare(const QSGMaterial *other) const {
  auto *o = static_cast<const VideoNv12Material *>(other);
  if (texY_ == o->texY_ && texUV_ == o->texUV_)
    return 0;
  if (texY_ < o->texY_)
    return -1;
  if (texY_ > o->texY_)
    return 1;
  if (texUV_ < o->texUV_)
    return -1;
  if (texUV_ > o->texUV_)
    return 1;
  return 0;
}

#pragma once

#include <QSGMaterial>
#include <QSGTexture>

class VideoNv12Material : public QSGMaterial {
public:
  VideoNv12Material();

  void setTextures(QSGTexture *texY, QSGTexture *texUV);
  void setSwapUV(bool swap) { swapUV_ = swap; }
  void setMatrix709(bool use709) { use709_ = use709; }
  void setFullRange(bool full) { fullRange_ = full; }
  bool swapUV() const { return swapUV_; }
  bool use709() const { return use709_; }
  bool fullRange() const { return fullRange_; }
  QSGTexture *textureY() const { return texY_; }
  QSGTexture *textureUV() const { return texUV_; }

  QSGMaterialType *type() const override;
  QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override;
  int compare(const QSGMaterial *other) const override;

private:
  QSGTexture *texY_ = nullptr;
  QSGTexture *texUV_ = nullptr;
  bool swapUV_ = false;
  bool use709_ = true;
  bool fullRange_ = false;
  static QSGMaterialType type_;
};

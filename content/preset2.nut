function preset2(seed)
{
  local col1 = [0.4, 0.6, 0.7, 1.0]
  local col2 = [0.1, 0.15, 0.2, 1.0]

  seedRandomDevice(1, seed)

  if("norm_tex" in this)
    destroyTexture(norm_tex)
  if("col_tex" in this)
    destroyTexture(col_tex)

  norm_tex <- createTexture(256, 256)
  local noisemap = createTexture(64, 64)

  
  ps <- makePointSet(1, 55, 2, 0.1)

  makeVoronoi(norm_tex, ps, 0.01)

  generateNoise(noisemap, 1)
  makeTurbulenceInplace(noisemap, 3)
  resizeTexture(noisemap, 256, 256)

  warpInplace(norm_tex, noisemap, 25.)
  destroyTexture(noisemap)
  
  
  tex2 <- createTexture(256, 256)
  noisemap = createTexture(64, 64)

  ps <- makePointSet(1, 75, 2, 0.1)

  makeVoronoi(tex2, ps, 0.005)

  generateNoise(noisemap, 1)
  makeTurbulenceInplace(noisemap, 3)
  resizeTexture(noisemap, 256, 256)

  warpInplace(tex2, noisemap, 25.)
  destroyTexture(noisemap)
  
  linFilter(tex2, 0., 1., 0.3, 0.0, 8)
  fillWithBlendChannel(norm_tex, [0.0, 0.0, 0.0, 0.0], tex2, 8, 0xf)
  destroyTexture(tex2)
  
  noisemap = createTexture(256, 256)
  fillTexture(noisemap, [0., 0., 0., 0.])
  generateWhiteNoise(noisemap, 1, 0x1)
  makeTurbulenceInplace(noisemap, 4, 1.5, 0x1)

  downsampleFilter(noisemap, 6, 0x1)
  linFilter(noisemap, 0.0, 1.0, 0.0, 0.5, 0x1)
  blurInplace(noisemap, [1, 1], 0x1)

  blendTextures(norm_tex, noisemap, 0xf, 0x1)

  makeNormalMap(norm_tex, 1.5)

  col_tex <- createTexture(256, 256);

  generateWhiteNoise(noisemap, 1, 0x1)
  makeTurbulenceInplace(noisemap, 5, 1.5, 0x1)

  fillTexture(col_tex, col2)
  fillWithBlendChannel(col_tex, col1, norm_tex, 1, 0xf)

  linFilter(norm_tex, 0., 1., 1., 0., 0x8)
  fillWithBlendChannel(col_tex, col2, norm_tex, 8, 0xf)
  linFilter(norm_tex, 0., 1., 1., 0., 0x8)

  bindSampler("normal_map", norm_tex)
  bindSampler("color_map", col_tex)
  
  destroyTexture(noisemap)
}
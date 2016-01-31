function preset1(seed)
{
  local col1 = [0.3, 0.6, 0.1, 1.0]
  local col2 = [1.0, 0.8, 0.8, 1.0]

  seedRandomDevice(1, seed)
  
  if("tex" in this)
    destroyTexture(tex)
  if("col_tex" in this)
    destroyTexture(col_tex)

  tex <- createTexture(256, 256)
  local noisemap = createTexture(64, 64)

  generateNoise(noisemap, 1)
  makeTurbulenceInplace(noisemap, 5)
  resizeTexture(noisemap, 256, 256)

  bl <- blob(256 * 256)

  for(local x = 0; x < 256; ++x)
  {
    local contrib = sin(20 * x * (PI / 256)) + 1
    contrib = ((contrib * contrib) * 63).tointeger()
    if(contrib >= 256) contrib = 255
    else if(contrib < 0) contrib = 0
    for(local y = 0; y < 256; ++y)
      bl.writen(contrib, 'b')
  }

  writeChannelb(tex, bl, 0x8)

  warpInplace(tex, noisemap, 250.)

  col_tex <- createTexture(256, 256)
  fillTexture(col_tex, col1)

  generateNoise(noisemap, 1)
  makeTurbulenceInplace(noisemap, 4)
  fillWithBlendChannel(col_tex, col2, noisemap, 8, 0xf)

  fillWithBlendChannel(col_tex, [0.2, 0.2, 0.2, 1.0], tex, 8, 0xf)

  linFilter(tex, 0., 1., 1., 0., 0x8)
  //fillTexture(tex, [0.5, 0.5, 0.5, 1.0])
  makeNormalMap(tex, 1.5)

  bindSampler("normal_map", tex)
  bindSampler("color_map", col_tex)
  
  destroyTexture(noisemap)
}
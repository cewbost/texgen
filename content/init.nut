loadNut("cube.nut")
loadNut("sphere.nut")

loadPipeline("pipeline.xml")
loadShader("model.shader")

col1 <- [0.4, 0.6, 0.7, 1.0]
col2 <- [0.2, 0.3, 0.35, 1.0]

seedRandomDevice(1, "HelloWorld")

tex <- createTexture(128, 128)
noisemap <- createTexture(32, 32)

ps <- makePointSet(1, 15, 2, 0.1)

makeVoronoi(tex, ps, -3.)

generateNoise(noisemap, 1)
makeTurbulenceInplace(noisemap, 3)
resizeTexture(noisemap, 128, 128)

warpInplace(tex, noisemap, 15.)

destroyTexture(noisemap)
noisemap <- createTexture(128, 128)
fillTexture(noisemap, [0., 0., 0., 0.])
generateWhiteNoise(noisemap, 1, 0x1)
makeTurbulenceInplace(noisemap, 6, 2., 0x1)

downsampleFilter(noisemap, 8, 0x1)
linFilter(noisemap, 0.0, 1.0, 0.0, 0.7, 0x1)
blurInplace(noisemap, [1, 1], 0x1)

blendTextures(tex, noisemap, 0xf, 0x1)

makeNormalMap(tex, 2.)

col_tex <- createTexture(128, 128);

//copyChannel(col_tex, 0xf, tex, 8)
generateWhiteNoise(noisemap, 1, 0x1)
makeTurbulenceInplace(noisemap, 5, 1.5, 0x1)

fillTexture(col_tex, col2)
fillWithBlendChannel(col_tex, col1, 0xf, tex, 1)

linFilter(tex, 0., 1., 1., 0., 0x8)
fillWithBlendChannel(col_tex, col2, 0xf, tex, 8)
linFilter(tex, 0., 1., 1., 0., 0x8)

/*fillTexture(tex, [1., 1., 1., 1.], 0xf)
makeNormalMap(tex, 2.)*/

bindSampler("normal_map", tex)
bindSampler("color_map", col_tex)
setModel(makeCube())

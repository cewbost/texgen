loadNut("cube.nut")
loadNut("sphere.nut")

loadPipeline("pipeline.xml")
loadShader("model.shader")

setModel(makeCube())

loadNut("preset1.nut")
loadNut("preset2.nut")

presets <- [preset1, preset2]

function loadPreset(idx, seed)
{
  --idx
  if(!(idx in presets))
  {
    print("preset " + idx + "does not exist.")
    return
  }
  
  presets[idx].call(getroottable(), seed)
}

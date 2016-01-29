function makeSphere(div)
{
  assert(type(div) == "integer" && div >= 0)
  
  local writeFloats = function(target, ...)
  {
    foreach(val in vargv)
      target.writen(val.tofloat(), 'f')
  }
  local writeInts = function(target, ...)
  {
    foreach(val in vargv)
      target.writen(val.tointeger(), 'i')
  }
  
  local fdiv = div + 1
  local divs = [0.]
  for(local x = 1; x < fdiv; ++x)
    divs.push(sin((PI / 2) * (x.tofloat() / fdiv.tofloat())))
  local rdivs = clone divs
  rdivs.reverse()
  divs.push(1.)
  divs.extend(rdivs)
  rdivs = divs.slice(1, -1)
  rdivs.apply(@(a) -a)
  divs.extend(rdivs)
  
  local verts_per_line = 3 + div * 2
  local num_verts = verts_per_line * verts_per_line * 2
  
  //verts
  local verts = blob(12 * num_verts)
  local tangs = blob(12 * num_verts)
  local texcoords = blob(8 * num_verts)
  
  for(local y = 0; y < verts_per_line; ++y)
  {
    local _y = divs[(y + 3 * fdiv) % divs.len()]
    for(local x = 0; x < verts_per_line; ++x)
    {
      local _x = divs[(x + 3 * fdiv) % divs.len()] * divs[y]
      local _z = divs[x] * divs[y]
      writeFloats(verts, _x, _y, _z)
      writeFloats(tangs, divs[x], 0., -divs[(x + 3 * fdiv) % divs.len()])
      writeFloats(texcoords,
        x.tofloat() / (verts_per_line - 1),
        y.tofloat() / (verts_per_line - 1))
    }
    for(local x = 0; x < verts_per_line; ++x)
    {
      local _x = divs[(x + 1 * fdiv) % divs.len()] * divs[y]
      local _z = divs[(x + 2 * fdiv) % divs.len()] * divs[y]
      writeFloats(verts, _x, _y, _z)
      writeFloats(tangs, divs[(x + 2 * fdiv) % divs.len()], 0., -divs[x + fdiv])
      writeFloats(texcoords,
        x.tofloat() / (verts_per_line - 1),
        y.tofloat() / (verts_per_line - 1))
    }
  }
  
  //indices
  local num_triangles = (verts_per_line - 1) * (verts_per_line - 1) * 4
  local indices = blob(num_triangles * 12)
  for(local y = 0; y < verts_per_line - 1; ++y)
  {
    for(local x = 0; x < verts_per_line - 1; ++x)
    {
      writeInts(indices,
        x + y * verts_per_line * 2,
        x + 1 + y * verts_per_line * 2,
        x + (y + 1) * verts_per_line * 2)
      writeInts(indices,
        x + 1 + y * verts_per_line * 2,
        x + 1 + (y + 1) * verts_per_line * 2
        x + (y + 1) * verts_per_line * 2)
    }
    for(local x = 0; x < verts_per_line - 1; ++x)
    {
      writeInts(indices,
        verts_per_line + x + y * verts_per_line * 2,
        verts_per_line + x + 1 + y * verts_per_line * 2,
        verts_per_line + x + (y + 1) * verts_per_line * 2)
      writeInts(indices,
        verts_per_line + x + 1 + y * verts_per_line * 2,
        verts_per_line + x + 1 + (y + 1) * verts_per_line * 2
        verts_per_line + x + (y + 1) * verts_per_line * 2)
    }
  }
  
  local sphere = {}
  
  sphere.vertices <- verts
  sphere.normals <- verts
  sphere.tangents <- tangs
  sphere.texcoords1 <- texcoords
  sphere.indices <- indices
  sphere.tanspace <- true
  
  return sphere
}

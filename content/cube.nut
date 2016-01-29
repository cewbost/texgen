function makeCube()
{
  local cube = {}

  //vertices
  local temp = blob(12 * 24)

  local writeVert3 = function(target, coords)
  {
    target.writen(coords[0], 'f')
    target.writen(coords[1], 'f')
    target.writen(coords[2], 'f')
  }
  
  local writeVert2 = function(target, coords)
  {
    target.writen(coords[0], 'f')
    target.writen(coords[1], 'f')
  }
  
  local writeQuad = function(target, coords)
  {
    writeVert3(target, coords[0])
    writeVert3(target, coords[1])
    writeVert3(target, coords[2])
    writeVert3(target, coords[3])
  }

  local writeSide = function(target, pitch, yaw)
  {
    local verts = [[-1, -1, -1], [-1, 1, -1], [1, 1, -1], [1, -1, -1]]
    
    switch(pitch)
    {
    case 1:
      foreach(vert in verts)
      {
        local v = -vert[0]
        vert[0] = vert[2]
        vert[2] = v
      }
      break
    case -1:
      foreach(vert in verts)
      {
        local v = vert[0]
        vert[0] = -vert[2]
        vert[2] = v
      }
      break
    case 2:
    case -2:
      foreach(vert in verts)
      {
        vert[0] = -vert[0]
        vert[2] = -vert[2]
      }
      break;
    }
    
    switch(yaw)
    {
    case 1:
      foreach(vert in verts)
      {
        local v = -vert[1]
        vert[1] = vert[2]
        vert[2] = v
      }
      break
    case -1:
      foreach(vert in verts)
      {
        local v = vert[1]
        vert[1] = -vert[2]
        vert[2] = v
      }
      break
    case 2:
    case -2:
      foreach(vert in verts)
      {
        vert[1] = -vert[1]
        vert[2] = -vert[2]
      }
      break;
    }
    
    writeQuad(target, verts)
  }
  
  writeSide(temp, 0, 0)
  writeSide(temp, 1, 0)
  writeSide(temp, 2, 0)
  writeSide(temp, -1, 0)
  writeSide(temp, 0, 1)
  writeSide(temp, 0, -1)

  cube.vertices <- temp

  //normals
  temp = blob(12 * 24)

  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 0., -1.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [-1., 0., 0.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 0., 1.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [1., 0., 0.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., -1., 0.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 1., 0.])

  cube.normals <- temp

  //tangents
  temp = blob(12 * 24)

  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 1., 0.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 1., 0.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 1., 0.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 1., 0.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 0., -1.])
  for(local x = 0; x < 4; ++x) writeVert3(temp, [0., 0., 1.])

  cube.tangents <- temp

  cube.tanspace <- true

  //texcoords1
  temp = blob(8 * 24)

  for(local x = 0; x < 6; ++x)
  {
    writeVert2(temp, [0., 0.])
    writeVert2(temp, [1., 0.])
    writeVert2(temp, [1., 1.])
    writeVert2(temp, [0., 1.])
  }

  cube.texcoords1 <- temp

  //indices
  temp = blob(24 * 6)

  for(local x = 0; x < 6; ++x)
  {
    temp.writen(x * 4 + 0, 'i')
    temp.writen(x * 4 + 1, 'i')
    temp.writen(x * 4 + 2, 'i')
    temp.writen(x * 4 + 0, 'i')
    temp.writen(x * 4 + 2, 'i')
    temp.writen(x * 4 + 3, 'i')
  }

  cube.indices <- temp
  
  return cube
}
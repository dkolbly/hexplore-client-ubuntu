#! /bin/bash
pngtopnm        terrain.png > terrain.rgb.pnm
pngtopnm -alpha terrain.png > terrain.alpha.pnm

pngtopnm        robot.png > robot.rgb.pnm
pngtopnm -alpha robot.png > robot.alpha.pnm

pngtopnm        turtle.png > turtle.rgb.pnm
pngtopnm -alpha turtle.png > turtle.alpha.pnm

pngtopnm        skybox.png > skybox.rgb.pnm
pngtopnm -alpha skybox.png > skybox.alpha.pnm

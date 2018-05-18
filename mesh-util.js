const { spawn } = require('child_process');

let packetInterval  = 0.1
let packetSize      = 512
let xsize           = 1
let ysize           = 2
let step            = 50
let numFlows        = 1
let seed            = 1
let seedLimit       = 33
/**
 * 802.11a/5GHz   = 1
 * 802.11b/2.4GHz = 2
 * 802.11g/2.4GHz = 3
 */
let standardPhy     = 1

const getPath = () => {
  return `../waf`; 
};

const getArgs = () => {
  const cmd = `mesh-RPG
        --x-size=${xsize}
        --y-size=${ysize}
        --step=${step}
        --packet-interval=${packetInterval}
        --packet-size=${packetSize}
        --numFlows=${numFlows}
        --seed=${seed}
        --standardPhy=${standardPhy}`;
  return [`--run`, `./${cmd}`];
};

const run = (path, args) => {
  const cmd = spawn(path, args);
  cmd.stdout.setEncoding('utf8');
  cmd.stderr.on('data', data => {
    console.log(`stderr: ${data}`);
  });
  cmd.stdout.on('data', data => {
    console.log(`stdout: ${data}`);
  });
};

const main = () => {
  experience(() => {
    for(let i = 0; i < seedLimit; i++){
      run(getPath(), getArgs());
      seed++;
    }
    seed = 1;
  });
};

/* EXPERIENCES */

const experience2Interval = (callback) => {
  packetInterval = 0.01;
  while(packetInterval <= 0.1){
    callback();
    packetInterval += 0.01;
  }
};

const experience3Size = (callback) => {
  packetInterval = 0.01;
  packetSize = 32;
  step = 100;
  while(packetSize <= 1024){
    callback();
    packetSize = packetSize * 2;
  }
};

const experience5Nodes = (callback) => {
  xsize = 2;
  ysize = 2;
  packetInterval = 0.01;
  while(packetInterval <= 0.1){
    while(xsize <= 10){
      callback();
      xsize += 1;
      ysize += 1;
    }
    packetInterval += 0.01;
  }
};

const experience6Flows = (callback) => {
  const xsizeLimit = 20;
  while(xsize <= xsizeLimit){
    callback();
    xsize += 1;
  }
};

const experience = experience5Nodes;

main();
// Hello OSC

// Intended to run alongside Bela: https://github.com/jarmitage/bela-paperprograms

importScripts('paper.js');
importScripts('osc.js');

function linexp(value, inMin, inMax, outMin, outMax, clamp = true) {
  if (clamp) {
    if (value <= inMin) { return outMin; }
    if (value >= inMax) { return outMax; }
  }

  return Math.pow(outMax / outMin, (value - inMin) / (inMax - inMin)) * outMin
}

function linlin(value, inMin, inMax, outMin, outMax, clamp = true) {
  if (clamp) {
    if (value <= inMin) { return outMin; }
    if (value >= inMax) { return outMax; }
  }

  return outMin + (((value - inMin) / (inMax - inMin)) * (outMax - outMin));
}

(async () => {
  // OSC example
  const osc = new OSC({ port: 8080 });

  let pressure;

  osc.onMessage = function(msg) {
    // console.log('hello-osc: ', msg);

    if (msg.address === "/bela/analogInputs/0" ){
      pressure = msg.args[0].value;
      console.log(pressure);
    }
    // osc.send({
    //   address : '/test1',
    //     args  : [
    //       {type: 'integer', value: 10},
    //       {type: 'float', value: 3.14}
    //     ]
    // });
  }

  // Get a canvas object for this paper.
  const canvas = await paper.get('canvas');

  // Draw "Hello world" on the canvas.
  const ctx = canvas.getContext('2d');
  ctx.font = '20px sans-serif';
  ctx.textAlign = 'center';
  ctx.fillStyle = 'rgb(255,0,0)';
  ctx.fillText('Hello', canvas.width / 2, canvas.height / 2 - 10);
  ctx.fillStyle = 'rgb(0,255,0)';
  ctx.fillText('world', canvas.width / 2, canvas.height / 2 + 20);
  ctx.commit();

  // Get a "supporter canvas", which is a canvas for the entire
  // projection surface.
  const supporterCanvas = await paper.get('supporterCanvas');
  const supporterCtx = supporterCanvas.getContext('2d');

  // Get the paper number of this piece of paper (which should not change).
  const myPaperNumber = await paper.get('number');

  // Repeat every 100 milliseconds.
  setInterval(async () => {

    // Get a list of all the papers.
    const papers = await paper.get('papers');

    // Clear out the supporter canvas. We get our own canvas, so we won't
    // interfere with other programs by doing this.
    supporterCtx.clearRect(0, 0, supporterCanvas.width, supporterCanvas.height);

    // Draw a circle in the center of our paper.
    const myCenter = papers[myPaperNumber].points.center;
    supporterCtx.fillStyle = supporterCtx.strokeStyle = 'rgb(0, 255, 255)';
    supporterCtx.beginPath();
    supporterCtx.arc(myCenter.x, myCenter.y, 100 * pressure, 0, 2*Math.PI);
    supporterCtx.fill();

    // Draw a line from our paper to each other paper.
    Object.keys(papers).forEach(otherPaperNumber => {
      if (otherPaperNumber !== myPaperNumber) {
        const otherCenter = papers[otherPaperNumber].points.center;

        supporterCtx.beginPath();
        supporterCtx.moveTo(myCenter.x, myCenter.y);
        supporterCtx.lineTo(otherCenter.x, otherCenter.y);
        supporterCtx.stroke();
      }
    });

    osc.send({
      address : '/bela/analogOutputs/0',
        args  : [
          {type: 'float', value: linlin(myCenter.x, 0, supporterCanvas.width, 0,1)},
          {type: 'float', value: linlin(myCenter.y, 0, supporterCanvas.height,30,1)}
        ]
    });

    osc.send({
      address : '/bela/analogOutputs/1',
        args  : [
          {type: 'float', value: linlin(myCenter.x, 0, supporterCanvas.width, 1,0)},
          {type: 'float', value: linlin(myCenter.y, 0, supporterCanvas.height,30,1)}
        ]
    });

    // Finally, commit to the canvas, which actually renders.
    supporterCtx.commit();
  }, 100);
})();

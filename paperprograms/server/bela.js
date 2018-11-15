const OSCWebSocketServer = require('./OSCWebSocketServer');

const defaultArgs = {
  localAddress:  '192.168.7.1', // Client's static IP when connected to Bela via Ethernet over USB
  localPort:     7563,
  remoteAddress: 'bela.local', // Bela's static IP when connected to client via Ethernet over USB
  remotePort:    7562,
  webSocketPort: 8080,
};

class Bela extends OSCWebSocketServer {
  constructor(args = defaultArgs) {
    super(args);
  }

  handleOscFromUdp(oscMessageOrBundle) {
    if (oscMessageOrBundle.oscType === 'message' &&
        oscMessageOrBundle.address === '/osc-setup') {
      this.sendOscToServer({ address: '/osc-setup-reply' });
    } else {
      this.sendOscToWebSocket(oscMessageOrBundle);
    }
  }

  start({ localAddress, localPort, remoteAddress, remotePort, webSocketPort } = defaultArgs) {
    super.start({ localAddress, localPort, remoteAddress, remotePort, webSocketPort });
  }
}

module.exports = { Bela, defaultArgs };

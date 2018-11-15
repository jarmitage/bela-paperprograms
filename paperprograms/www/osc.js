class OSC {

	constructor({port=8080}){
		this.ws = new WebSocket(`ws://${location.hostname}:${port}`);
		this.ws.onopen = () => {
			console.log("osc.js: opened");
		};
		this.ws.onmessage = event => {
			if ('data' in event) {
				this.onMessage(JSON.parse(event.data));
			}
		}
	}

	onMessage(data){
		console.log(data);
	}

	send(msg){
		this.ws.send(JSON.stringify(msg));
	}
}

(function(workerContext) {
	if (workerContext.OSC !== undefined) return;

	workerContext.OSC = OSC;

})(self);
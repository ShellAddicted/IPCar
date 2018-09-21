
const DIRECTIONS = { Backward: 1, Forward: 2, Brake: 3, Release: 4 };
const GEARS = { 1: "R", 2: "D", 3: "B", 4: "N" };

// GamePad Emulates keyboard
const GAMEPAD_KEYS_MAP = { 0: " ", 6: "s", 7: "w", 1: "z", 14: "a", 15: "d" };

// Maximum throttle allowed using Gamepad proportional buttons (Example: RT/LT on Xbox controller)
let throttleLimit = 100;
//Gamepad Analog Direction (X Axis %)
let xAxis = 0;

//Tasks handle
let gamePadTask = 0;
let telemetryTask = 0;
let controlsTask = 0;

let throttle = 75;
let currentKeys = []; //Dynamic Array containing current pressed keys

const chartConfig = {
	type: 'line',
	backgroundColor: 'rgba(0,0,0,1)',
	data: {
		labels: [],
		datasets: [{
			label: 'Throttle (x10%)',
			backgroundColor: 'rgba(255,0,0,1)',
			borderColor: 'rgba(255,0,0,1)',
			data: [
			],
			fill: false,
		},
		{
			label: 'Speed (km/h)',
			fill: false,
			backgroundColor: 'rgba(0,0,255,1)',
			borderColor: 'rgba(0,0,255,1)',
			data: [
			],
		},
		{
			label: 'Steering (x10%)',
			fill: false,
			backgroundColor: 'rgba(255,255,255,1)',
			borderColor: 'rgba(255,255,255,1)',
			data: [
			],
		},
		{
			label: 'Linear Accel X (m/s²)',
			fill: false,
			backgroundColor: 'rgba(0,255,0,1)',
			borderColor: 'rgba(0,255,0,1)',
			data: [
			],
		},
		{
			label: 'Linear Accel Y (m/s²)',
			fill: false,
			backgroundColor: 'rgba(0,191,255,1)',
			borderColor: 'rgba(0,191,255,1)',
			data: [
			],
		},
		{
			label: 'Linear Accel Z (m/s²)',
			fill: false,
			backgroundColor: 'rgba(255,255,0,1)',
			borderColor: 'rgba(255,255,0,1)',
			data: [
			],
		},
		{
			label: 'Battery (V)',
			fill: false,
			backgroundColor: 'rgba(255,0,255,1)',
			borderColor: 'rgba(255,0,255,1)',
			data: [
			],
		}
		]
	},
	options: {
		responsive: true,
		legend: {
			display: false
		},
		tooltips: {
			mode: 'index',
			intersect: false,
		},
		hover: {
			mode: 'nearest',
			intersect: true
		},
		scales: {
			xAxes: [{
				display: true,
			}],
			yAxes: [{
				ticks: {
					min: -15,
					max: 15,
					stepSize: 1
				},
				display: true,
			}]
		}
	}
};


class wsConnection {
	constructor(path, wsReconnect = true, ID = "") {
		this.ws = null;
		this.path = path;
		this.wsReconnect = wsReconnect;
		this.ID = ID;
	}

	connect() {
		this.ws = new WebSocket(this.path);
		this.ws.onclose = (evt) => {
			if (this.wsReconnect) {
				connect();
			}
		};
		this.ws.onmessage = (evt) => {
			this.handleMessage(evt.data);

		};
		this.ws.onerror = (evt) => {
			console.error(this.ID + " " + evt);
		};
	}

	getState() {
		return this.ws.readyState;
	}

	wsReq(data, callback) {
		this.ws.onmessage = (evt) => {
			callback(evt.data);
		}
		this.ws.send(data);
	}

	handleMessage(data) {
		//override this if you need
		//this allows to handle unexpected messages from server
		// **(unexpected means not requested using wsReq() )
		console.log(data);
	}

	close() {
		this.wsReconnect = false;
		this.ws.close();
	}
}

const controlsConnection = new wsConnection("ws://" + document.location.host + "/ws", true, "controls");
const telemetryConnection = new wsConnection("ws://" + document.location.host + "/ws", true, "telemetry");

function prettifyTimestamp(ms, decimals = 0) {
	var minutes = Math.floor(ms / 60000);
	var seconds = ((ms % 60000) / 1000).toFixed(decimals);
	return minutes + ":" + (seconds < 10 ? '0' : '') + seconds;
}

function arrayHasValue(what, where) {
	return (where.indexOf(what) >= 0);
}

function getDirection() {
	let Direction = DIRECTIONS.Release;
	let Angle = 0;
	let Speed = throttle;

	let brakeY = arrayHasValue(" ", currentKeys);
	let forward = arrayHasValue("w", currentKeys);
	let backward = arrayHasValue("s", currentKeys);
	let left = arrayHasValue("a", currentKeys);
	let right = arrayHasValue("d", currentKeys);

	if (brakeY) {
		Direction = DIRECTIONS.Brake;
		Speed = 0;
	}
	else if ((forward && backward) || (!forward && !backward)) {
		Direction = DIRECTIONS.Release;
		Speed = 0;
	}
	else if (forward) {
		Direction = DIRECTIONS.Forward;
	}
	else if (backward) {
		Direction = DIRECTIONS.Backward;
	}

	if (Math.abs(xAxis) > 1) {
		Angle = xAxis * -1;
	}
	else if ((left && right) || (!left && !right)) {
		Angle = 0;
	}
	else if (left) {
		Angle = +100;
	}
	else if (right) {
		Angle = -100;
	}
	cmd = { "cmd": "run", "args": [Direction, Speed, Angle] };
	return cmd;
}

function driveTask() {
	document.getElementById("linkState").innerHTML = ((controlsConnection.getState() == WebSocket.OPEN) ? "Online" : "Offline");
	tmp = getDirection();
	controlsConnection.wsReq(JSON.stringify(tmp), (data) => {
		//console.log("Sent -->>" + JSON.stringify(tmp));
	});
}

function readGamepad() {
	let connectedGamePads = navigator.getGamepads();
	if (connectedGamePads.length > 0) {
		if (connectedGamePads[0] != null) {
			currentGamepad = connectedGamePads[0];

			xAxis = parseInt((connectedGamePads[0].axes[0]) * 100);
			for (var i = 0; i < currentGamepad.buttons.length; i++) {
				if (currentGamepad.buttons[i].pressed === true) {
					if ((i == 6) || (i == 7)) {
						throttle = parseInt((currentGamepad.buttons[i].value * 100));
						if (throttle >= throttleLimit) {
							throttle = throttleLimit;
						}
					}
					if (!arrayHasValue(GAMEPAD_KEYS_MAP[i], currentKeys)) {
						currentKeys.push(GAMEPAD_KEYS_MAP[i]);
					}
				}

				else if (currentGamepad.buttons[i].pressed === false && arrayHasValue(GAMEPAD_KEYS_MAP[i], currentKeys) === true) {
					currentKeys.splice(currentKeys.indexOf(GAMEPAD_KEYS_MAP[i]), 1);
				}
			}
			driveTask();
		}
	}
}

function updateChart(timestamp, throttle, steering, speed, lia, vBat) {
	let samples = 20;
	chartConfig.data.labels.push(prettifyTimestamp(timestamp, 2))

	chartConfig.data.datasets[0].data.push({ x: timestamp, y: throttle / 10 }); // scale it
	chartConfig.data.datasets[1].data.push({ x: timestamp, y: speed });
	chartConfig.data.datasets[2].data.push({ x: timestamp, y: steering });

	if (lia != null) {
		chartConfig.data.datasets[3].data.push({ x: timestamp, y: lia["x"] });
		chartConfig.data.datasets[4].data.push({ x: timestamp, y: lia["y"] });
		chartConfig.data.datasets[5].data.push({ x: timestamp, y: lia["z"] });
	}
	if (vBat != null) {
		chartConfig.data.datasets[6].data.push({ x: timestamp, y: vBat });
	}

	if (chartConfig.data.labels.length > samples) {
		chartConfig.data.labels = chartConfig.data.labels.slice(-samples);
		chartConfig.data.datasets.forEach(function (currentDataset) {
			currentDataset.data = currentDataset.data.slice(-samples);
		});
	}
	window.myLine.update({ duration: 10 });
}

function getTelemetry() {
	telemetryConnection.wsReq(JSON.stringify({ "cmd": "getTelemetry", "args": [] }), (jdata) => {
		let data = JSON.parse(jdata);
		if (data == undefined) {
			return;
		}
		let record = data.telemetry.slice(-1)[0];
		document.getElementById("carUptime").innerHTML = prettifyTimestamp(parseInt(record.uptime));

		document.getElementById("carGear").innerHTML = GEARS[record.direction];
		document.getElementById("carThrottle").innerHTML = record.throttle;
		document.getElementById("carSteeringPercentage").innerHTML = record.steering;

		let carSpeed = ((record.wheels[2].success) ? (record.wheels[2].speed * 3.6).toFixed(2) : null);
		let carRPM = ((record.wheels[2].success) ? record.wheels[2].rpm : null);
		let carBatteryVoltage = ((record.battery.success) ? (record.battery.voltage / 1000).toFixed(1) : null);
		let carBatteryPercentage = ((record.battery.success) ? record.battery.percentage : null);

		let lia = ((record.imu.success) ? record.imu.lia : null);

		document.getElementById("carSpeed").innerHTML = carSpeed;
		document.getElementById("carRPM").innerHTML = carRPM;
		document.getElementById("batteryVoltage").innerHTML = carBatteryVoltage;
		document.getElementById("batteryPercentage").innerHTML = carBatteryPercentage;

		updateChart(parseInt(record.uptime), record.throttle, record.steering, carSpeed, lia, carBatteryVoltage);
	});
}

controlsConnection.connect();
telemetryConnection.connect();
telemetryTask = window.setInterval(getTelemetry, 100);
gamePadTask = window.setInterval(readGamepad, 100);
controlsTask = window.setInterval(driveTask, 50);

window.onkeydown = (evt) => {
	if (!arrayHasValue(evt.key, currentKeys)) {
		currentKeys.push(evt.key);
	}
	driveTask();
};

window.onkeyup = (e) => {
	if (arrayHasValue(evt.key, currentKeys)) {
		currentKeys.splice(currentKeys.indexOf(evt.key), 1);
	}
	driveTask();
};

window.onblur = (e) => {
	currentKeys = [];
};

window.onfocus = (e) => {
	currentKeys = [];
};

window.onload = () => {
	let ctx = document.getElementById('chartA').getContext('2d');
	window.myLine = new Chart(ctx, chartConfig);
};
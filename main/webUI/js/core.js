Directions = {Backward: 1, Forward: 2, Brake: 3, Release: 4};
gears = {1:"R", 2:"D", 3:"B", 4:"N"};

// GamePad Emulates keyboard
GameKeys = {0:" ", 6:"s", 7:"w", 1:"z", 14:"a", 15:"d"};
throttleLimit = 100;
currentKeys = [];

gamePadTask = 0;
telemetryTask = 0;
controlsTask = 0;
Xaxis = 0;
throttle = 75;

class wsConnection{
    constructor(path, wsReconnect = true, ID=""){
        this.ws = null;
        this.path = path;
        this.wsReconnect = wsReconnect;
        this.ID=ID;
    }

    connect(){
        this.ws = new WebSocket(this.path);
        this.ws.onopen = function(evt) {
            console.log(this.ID + "WSopen");
        };

        this.ws.onclose = function(evt) {
            console.log(this.ID + "WSclose");
            if (this.wsReconnect){
                connect();
            }
        };
        this.ws.onmessage = function(evt) {
            console.log(evt.data);
        };
        this.ws.onerror = function(evt) {
            console.log(this.ID + "err: "+ evt);
        };
    }

    getState(){
        return this.ws.readyState;
    }

    wsReq(data, callback){
        this.ws.onmessage = function(evt){
            callback(evt.data);
        }
        this.ws.send(data);
    }

    close(){
        if (this.ws != null){
            this.wsReconnect = false;
            this.ws.close();
        }
    }
}
controlsConnection = new wsConnection("ws://" + document.location.host + "/ws", true, "controls");
telemetryConnection = new wsConnection("ws://" + document.location.host + "/ws", true, "telemetry");

function array_has_value(what, where){
    return (where.indexOf(what) >= 0);
}

function enableFeed(){
    telemetryTask = window.setInterval(getTelemetry, 100);
}

function disableFeed(){
    clearInterval(telemetryTask);
}

function driveTask(){
    document.getElementById("linkState").innerHTML = ((controlsConnection.getState() == WebSocket.OPEN) ? "Online" : "Offline");
    tmp = getDirection();
    controlsConnection.wsReq(JSON.stringify(tmp),function(data){
        console.log("Sent -->>"+JSON.stringify(tmp));
    });
}

function getDirection() {
    var Direction = Directions.Release;
    var Angle = 0;
    var Speed = throttle;

    var brakeY = array_has_value("z", currentKeys);
    var forward = array_has_value("w", currentKeys);
    var backward = array_has_value("s", currentKeys);
    var left = array_has_value("a", currentKeys);
    var right = array_has_value("d", currentKeys);
    var stop = array_has_value(" ", currentKeys);

    if (brakeY){
        Direction = Directions.Brake;
        Speed = 0;
    }
    else if ((forward && backward) || (!forward && !backward)){
        Direction = Directions.Release;
        Speed = 0;
    }
    else if (forward) {
        Direction = Directions.Forward;
    }
    else if (backward){
        Direction = Directions.Backward;
    }

    if(Math.abs(Xaxis) > 1){
        Angle = Xaxis*-1;
    }
    else if ((left && right) || (!left && !right)){
        Angle = 0;
    }
    else if (left) {
        Angle = +100;
    }
    else if (right){
        Angle = -100;
    }
    cmd = {"cmd":"run","args":[Direction, Speed, Angle]};
    return cmd;
}

function readGamepad(){
    Gpads = navigator.getGamepads();
    if (Gpads.length > 0){
        if (Gpads[0] != null){
            currentGamepad = Gpads[0];

            Xaxis = parseInt((Gpads[0].axes[0])*100);
            for (var i=0; i < currentGamepad.buttons.length; i++){
                if (currentGamepad.buttons[i].pressed === true){
                    if ((i==6) || (i==7)){
                        throttle = parseInt((currentGamepad.buttons[i].value*100));
                        if (throttle >= throttleLimit){
                            throttle = throttleLimit;
                        }
                    }
                    if (!array_has_value(GameKeys[i], currentKeys)){
                        currentKeys.push(GameKeys[i]);
                    }
                }

                else if (currentGamepad.buttons[i].pressed === false && array_has_value(GameKeys[i], currentKeys) === true){
                    currentKeys.splice(currentKeys.indexOf(GameKeys[i]), 1);
                }
            }
            driveTask();
        }
    }
}

function getTelemetry(){
    telemetryConnection.wsReq(JSON.stringify({"cmd":"getTelemetry","args": []}),function(dq){
        var data = JSON.parse(dq);
        if (data == undefined){
            return;
        }
        record = data.telemetry.slice(-1)[0];
        document.getElementById("carUptime").innerHTML = millisToMinutesAndSeconds(parseInt(record.uptime));
        
        document.getElementById("carGear").innerHTML = gears[record.direction];
        document.getElementById("carThrottle").innerHTML = record.throttle;
        document.getElementById("carSteeringPercentage").innerHTML = record.steering;
    
        var carSpeed = ((record.wheels[2].success) ? (record.wheels[2].speed*3.6).toFixed(2) : null);
        var carRPM = ((record.wheels[2].success) ? record.wheels[2].rpm : null);
        var carBatteryVoltage = ((record.battery.success) ? (record.battery.voltage/1000).toFixed(1) : null);
        var carBatteryPercentage = ((record.battery.success) ? record.battery.percentage : null);
    
        var lia = ((record.imu.success) ? record.imu.lia : null); 
        
        document.getElementById("carSpeed").innerHTML = carSpeed;
        document.getElementById("carRPM").innerHTML = carRPM;
        document.getElementById("batteryVoltage").innerHTML = carBatteryVoltage;
        document.getElementById("batteryPercentage").innerHTML = carBatteryPercentage;
    
        updateChart(parseInt(record.uptime), record.throttle, record.steering, carSpeed, lia, carBatteryVoltage);
    });
}

function controlsMain(){
    controlsConnection.connect();
    telemetryConnection.connect();
    gamePadTask = window.setInterval(readGamepad, 100);
    controlsTask = window.setInterval(driveTask, 50);

    window.onkeydown = function(e) {
        if (array_has_value(e.key, currentKeys) === false) {
            currentKeys.push(e.key);
        }
        driveTask();
    };

    window.onkeyup = function(e){
        if (array_has_value(e.key, currentKeys) === true) {
            currentKeys.splice(currentKeys.indexOf(e.key), 1);
        }
        driveTask();
    };

    window.onblur = function(){
        controlsConnection.close();
        telemetryConnection.close();
        currentKeys = [];
    };

    window.onfocus = function(){
        controlsConnection.connect();
        telemetryConnection.connect();
    };
}

controlsMain();
enableFeed();

function millisToMinutesAndSeconds(millis, decimals = 0) {
    var minutes = Math.floor(millis / 60000);
    var seconds = ((millis % 60000) / 1000).toFixed(decimals);
    return minutes + ":" + (seconds < 10 ? '0' : '') + seconds;
}

var config = {
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
        legend:{
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
                ticks:{
                    min: -15,
                    max: 15,
                    stepSize: 1
                },
                display: true,
            }]
        }
    }
};

window.onload = function() {
    var ctx = document.getElementById('chartA').getContext('2d');
    window.myLine = new Chart(ctx, config);
};

function updateChart(timestamp, throttle, steering, speed,lia, vBat){
    var samples = 20;
    config.data.labels.push(millisToMinutesAndSeconds(timestamp,2))
    
    config.data.datasets[0].data.push({x:timestamp, y:throttle/10}); // scale it
    config.data.datasets[1].data.push({x:timestamp, y:speed});
    config.data.datasets[2].data.push({x:timestamp, y:steering});
    
    if (lia != null){
        config.data.datasets[3].data.push({x:timestamp, y:lia["x"]});
        config.data.datasets[4].data.push({x:timestamp, y:lia["y"]});
        config.data.datasets[5].data.push({x:timestamp, y:lia["z"]});
    }
    if (vBat != null){
        config.data.datasets[6].data.push({x:timestamp, y:vBat});
    }
    
    if (config.data.labels.length > samples){
        config.data.labels = config.data.labels.slice(-samples);
        config.data.datasets.forEach(function(currentDataset){
            currentDataset.data = currentDataset.data.slice(-samples);
        });
    }
    window.myLine.update({duration: 10});
}
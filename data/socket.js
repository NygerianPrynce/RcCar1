var socket;


function initWebSocket() {
  if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
    return; // Don't create a new socket if one is already open or connecting
  }

  socket = new WebSocket(`ws://${window.location.hostname}/ws`);

  socket.onopen = () => {
    console.log('✅ WebSocket connected');
  };

  socket.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if ('speed' in data) {
        currentSpeed = data.speed;
        document.getElementById('sV').textContent = data.speed;
        sliderSpeed.value = data.speed;
        inputSpeed.value = data.speed;
      }
      if ('steering' in data) {
        currentSteer = data.steering;
        document.getElementById('stV').textContent = data.steering;
        sliderSteer.value = data.steering;
        inputSteer.value = data.steering;
      }
      if ('distance' in data) {
        document.getElementById('uV').textContent = data.distance + " cm";
      }
    } catch (e) {
      console.error("Failed to parse message as JSON:", event.data);
    }
  };

  socket.onclose = () => {
    console.log('WebSocket disconnected, retrying in 2s');
    socket = null; // Clear the socket reference so reconnect works
    setTimeout(initWebSocket, 2000);
  };

  socket.onerror = (e) => {
    console.error("WebSocket error:", e);
    socket.close();
  };
}

function initControls() {
  sliderSpeed = document.getElementById('speedSlider');
  inputSpeed = document.getElementById('speedInput');
  sliderSpeed.oninput = () => sendSpeed(slider.value);
  inputSpeed.oninput = () => sendSpeed(input.value);

  sliderSteer = document.getElementById('steeringSlider');
  inputSteer = document.getElementById('steeringInput');
  sliderSteer.oninput = () => sendSteer(slider.value);
  inputSteer.oninput = () => sendSteer(input.value);
}

function sendSpeed(value) {

    value = parseInt(value) || 0;

  // Mata zone
  if (value > -80 && value < 80) value = 0;

  value = Math.max(-255, Math.min(255, value));
  currentSpeed = value;

  sliderSpeed.value = value;
  inputSpeed.value = value;
  document.getElementById('sV').textContent = value;
    
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ speed: parseInt(value) }));
    }

}

function sendSteer(value) {
    value = Math.max(500, Math.min(2500, parseInt(value) || 0));
    currentSteer = value;
    sliderSteer.value = value;
    inputSteer.value = value;
    document.getElementById('stV').textContent = value;
    if (socket && socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ steering: parseInt(value) }));
    }
}

let sliderSpeed, inputSpeed, sliderSteer, inputSteer;
let currentSpeed = 0;
let currentSteer = 1500;
let speedAdjustInterval = null;
let steerAdjustInterval = null;
let speedReturnInterval = null;
let steerReturnInterval = null;
const speedStep = 18;
const steerStep = 125;
const intervalDelaySpeed = 25;
const intervalDelaySteer = 25;
function startSpeedAdjust(direction) {
  if (speedAdjustInterval) return;
  speedAdjustInterval = setInterval(() => {
    if (direction === 'up') {
      currentSpeed = Math.min(currentSpeed + speedStep, 255);
      if (currentSpeed < 80) currentSpeed = 80;
    } else if (direction === 'down') {
      currentSpeed = Math.max(currentSpeed - speedStep, -255);
      if (currentSpeed > -80) currentSpeed = -80;
    }
    sendSpeed(currentSpeed);
  }, intervalDelaySpeed);
}

function startSteerAdjust(direction) {
  if (steerAdjustInterval) return;
  steerAdjustInterval = setInterval(() => {
    if (direction === 'right') {
      currentSteer = Math.min(currentSteer + steerStep, 2500);
    } else if (direction === 'left') {
      currentSteer = Math.max(currentSteer - steerStep, 500);
    }
    sendSteer(currentSteer);
  }, intervalDelaySteer);
}


function stopSpeedAdjust() {
  clearInterval(speedAdjustInterval);
  speedAdjustInterval = null;

  clearInterval(speedReturnInterval);
  speedReturnInterval = setInterval(() => {
    if (currentSpeed > 0) currentSpeed -= (speedStep+20);
    else if (currentSpeed < 0) currentSpeed += (speedStep+20);

    if (Math.abs(currentSpeed) < speedStep) {
      currentSpeed = 0;
      clearInterval(speedReturnInterval);
    }
    sendSpeed(currentSpeed);
  }, intervalDelaySpeed);
}

function stopSteerAdjust() {
  clearInterval(steerAdjustInterval);
  steerAdjustInterval = null;

  clearInterval(steerReturnInterval);
  steerReturnInterval = setInterval(() => {
    if (currentSteer > 1500) currentSteer -= steerStep;
    else if (currentSteer < 1500) currentSteer += steerStep;

    if (Math.abs(currentSteer - 1500) < steerStep) {
      currentSteer = 1500;
      clearInterval(steerReturnInterval);
    }
    sendSteer(currentSteer);
  }, intervalDelaySteer);
}



window.onload = function () {
  initControls();
  initWebSocket();
      console.log("Window loaded, initializing controls and websockets");

  document.addEventListener('keydown', (event) => {
  const key = event.key.toLowerCase();
  if (['input', 'textarea'].includes(document.activeElement.tagName.toLowerCase())) return;

  if (key === 'w') startSpeedAdjust('up');
  else if (key === 's') startSpeedAdjust('down');
  else if (key === 'a') startSteerAdjust('left');
  else if (key === 'd') startSteerAdjust('right');
});

document.addEventListener('keyup', (event) => {
  const key = event.key.toLowerCase();
  if (key === 'w' || key === 's') stopSpeedAdjust();
  if (key === 'a' || key === 'd') stopSteerAdjust();
});

window.addEventListener("beforeunload", () => {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.close();
  }
});


}
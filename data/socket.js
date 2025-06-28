
// Session timer
let sessionStartTime = Date.now();
setInterval(() => {
  const elapsed = Math.floor((Date.now() - sessionStartTime) / 1000);
  const minutes = Math.floor(elapsed / 60).toString().padStart(2, '0');
  const seconds = (elapsed % 60).toString().padStart(2, '0');
  document.getElementById('sessionTime').textContent = `TIME: ${minutes}:${seconds}`;
}, 1000);

// Your existing WebSocket code
var socket;

function initWebSocket() {
  if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
    return;
  }

  socket = new WebSocket(`ws://${window.location.hostname}/ws`);

  socket.onopen = () => {
    console.log('✅ WebSocket connected');
    document.getElementById('connectionStatus').className = 'status-indicator';
    document.getElementById('telemetryStatus').className = 'status-indicator';
  };

  socket.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      if ('speed' in data) {
        currentSpeed = data.speed;
        const absPercent = Math.abs(Math.round(mapRange(data.speed, -255, 255, -100, 100)));

        let labelText;
        if (Math.abs(data.speed) < 80) {
          labelText = "Idle";
        } else if (data.speed > 0) {
          labelText = `Drive: ${absPercent}%`;
        } else {
          labelText = `Reverse: ${absPercent}%`;
        }

        document.getElementById('sV').textContent = labelText;
        sliderSpeed.value = data.speed;
      }
      if ('steering' in data) {
        currentSteer = data.steering;

        const angle = Math.round(mapRange(data.steering, 500, 2500, -45, 45));  // Steering angle from -45 to 45
        let steerLabel;

        if (Math.abs(angle) < 5) {
          steerLabel = "Straight";
        } else if (angle > 0) {
          steerLabel = `Right: ${angle}°`;
        } else {
          steerLabel = `Left: ${Math.abs(angle)}°`;
        }

        document.getElementById('stV').textContent = steerLabel;
        sliderSteer.value = data.steering;
      }
      if ('distance' in data) {
        document.getElementById('uV').innerHTML = data.distance + '<span class="data-unit">CM</span>';
      }
      if ('ax' in data) {
        const gForce = (data.ax / 9.81).toFixed(2);
        document.getElementById('ax').innerHTML = gForce + '<span class="data-unit">G</span>';
      }
      if ('velocity' in data) {
        document.getElementById('velocity').innerHTML = Math.abs(data.velocity).toFixed(1) + '<span class="data-unit">MPH</span>';
      }
      if ('b1Percent' in data) {
        document.getElementById('b1Percent').innerHTML = data.b1Percent;
        updateBatteryLevel("b1Percent", data.b1Percent);
      }
      if ('b2Percent' in data) {
        document.getElementById('b2Percent').innerHTML = data.b2Percent;
        updateBatteryLevel("b2Percent", data.b2Percent);
      }
      const pitch = 'pitch' in data ? data.pitch : 0;
      const roll = 'roll' in data ? data.roll : 0;

      document.getElementById("pitchVal").textContent = pitch.toFixed(1) + "°";
      document.getElementById("rollVal").textContent = roll.toFixed(1) + "°";

    } catch (e) {
      console.error("Failed to parse message as JSON:", event.data);
    }
  };

  socket.onclose = () => {
    console.log('WebSocket disconnected, retrying in 2s');
    document.getElementById('connectionStatus').className = 'status-indicator error';
    document.getElementById('telemetryStatus').className = 'status-indicator error';
    socket = null;
    setTimeout(initWebSocket, 2000);
  };

  socket.onerror = (e) => {
    console.error("WebSocket error:", e);
    document.getElementById('connectionStatus').className = 'status-indicator error';
    socket.close();
  };
}

function initControls() {
  sliderSpeed = document.getElementById('speedSlider');
  sliderSpeed.oninput = () => sendSpeed(sliderSpeed.value);

  sliderSteer = document.getElementById('steeringSlider');
  sliderSteer.oninput = () => sendSteer(sliderSteer.value);
}

function sendSpeed(value) {
  value = parseInt(value) || 0;
  if (value > -80 && value < 80) value = 0;
  value = Math.max(-255, Math.min(255, value));
  currentSpeed = value;
  sliderSpeed.value = value;
  const absPercent = Math.abs(Math.round(mapRange(value, -255, 255, -100, 100)));
  let labelText;
  if (Math.abs(value) < 80) {
    labelText = "Idle";
  } else if (value > 0) {
    labelText = `Drive: ${absPercent}%`;
  } else {
    labelText = `Reverse: ${absPercent}%`;
  }
  document.getElementById('sV').textContent = labelText;
  
  // Update holographic progress visualization
  const progress = document.getElementById('throttleProgress');
  const percentage = Math.abs(value) / 255 * 100;
  progress.style.width = percentage + '%';
  
  if (value >= 0) {
    progress.style.left = '50%';
    progress.style.background = `
      linear-gradient(90deg, 
        transparent 0%,
        rgba(255, 69, 0, 0.3) 20%,
        rgba(255, 140, 0, 0.6) 50%,
        rgba(255, 200, 0, 0.8) 80%,
        rgba(255, 255, 255, 0.4) 100%)`;
  } else {
    progress.style.left = (50 - percentage/2) + '%';
    progress.style.background = `
      linear-gradient(90deg, 
        rgba(255, 255, 255, 0.4) 0%,
        rgba(255, 0, 68, 0.8) 20%,
        rgba(255, 69, 0, 0.6) 50%,
        rgba(255, 140, 0, 0.3) 80%,
        transparent 100%)`;
  }
  
  // Add particle effect on value change
  createParticles('throttle');
  
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ speed: parseInt(value) }));
  }
}

function sendSteer(value) {
  value = Math.max(500, Math.min(2500, parseInt(value) || 0));
  currentSteer = value;
  sliderSteer.value = value;
  const angle = Math.round(mapRange(value, 500, 2500, -45, 45));
  let steerLabel;
  if (Math.abs(angle) < 5) {
    steerLabel = "Straight";
  } else if (angle > 0) {
    steerLabel = `Right: ${angle}°`;
  } else {
    steerLabel = `Left: ${Math.abs(angle)}°`;
  }
  document.getElementById('stV').textContent = steerLabel;
  
  // Update holographic steering progress visualization
  const progress = document.getElementById('steeringProgress');
  const centerValue = 1500;
  const range = value > centerValue ? 2500 - centerValue : centerValue - 500;
  const offset = Math.abs(value - centerValue);
  const percentage = (offset / range) * 50;
  
  if (value > centerValue) {
    progress.style.left = '50%';
    progress.style.width = percentage + '%';
  } else if (value < centerValue) {
    progress.style.left = (50 - percentage) + '%';
    progress.style.width = percentage + '%';
  } else {
    progress.style.width = '0%';
  }
  
  // Add particle effect on value change
  createParticles('steering');
  
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ steering: parseInt(value) }));
  }
}

// Holographic particle effect system
function createParticles(type) {
  const container = document.querySelector(`.control-block.${type} .slider-container`);
  const colors = type === 'throttle' ? 
    ['rgba(255, 69, 0, 0.8)', 'rgba(255, 140, 0, 0.6)', 'rgba(255, 200, 0, 0.4)'] :
    ['rgba(0, 191, 255, 0.8)', 'rgba(0, 150, 255, 0.6)', 'rgba(100, 200, 255, 0.4)'];
  
  for (let i = 0; i < 3; i++) {
    setTimeout(() => {
      const particle = document.createElement('div');
      particle.className = 'particle';
      particle.style.background = colors[Math.floor(Math.random() * colors.length)];
      particle.style.left = Math.random() * 100 + '%';
      particle.style.bottom = '0px';
      container.appendChild(particle);
      
      setTimeout(() => {
        if (particle.parentNode) {
          particle.parentNode.removeChild(particle);
        }
      }, 6000);
    }, i * 200);
  }
}

let sliderSpeed, sliderSteer;
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
    if (currentSpeed > 0) currentSpeed -= (speedStep + 20);
    else if (currentSpeed < 0) currentSpeed += (speedStep + 20);
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
  console.log("F1 Telemetry Dashboard initialized");

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

function updateBatteryLevel(elementId, percentage) {
  const el = document.getElementById(elementId);
  const percentValue = parseInt(percentage);
  el.textContent = percentValue + "%";

  // Remove any existing status class
  el.classList.remove("battery-high", "battery-medium", "battery-low");

  // Apply based on thresholds
  if (percentValue > 60) {
    el.classList.add("battery-high");
  } else if (percentValue >= 30) {
    el.classList.add("battery-medium");
  } else {
    el.classList.add("battery-low");
  }
}

function mapRange(value, inMin, inMax, outMin, outMax) {
  return ((value - inMin) * (outMax - outMin)) / (inMax - inMin) + outMin;
}


// Initialize EmailJS with your Public Key
emailjs.init("7zCUASgq7E5YcYAdj");

const ESP_IP = "192.168.196.234"; // Replace with your ESP8266 IP
let lastPirState = false;
let dataRefreshInterval;
let connectionRetries = 0;
const MAX_RETRIES = 5;

// Initialize gauges
let gaugeTemp = new JustGage({
    id: "gauge_temp",
    value: 0,
    min: 0,
    max: 50,
    title: "Temperature",
    label: "°C",
    gaugeWidthScale: 0.6,
    levelColors: ["#00ff00", "#ffcc00", "#ff0000"],
    counter: true
});

let gaugeHumid = new JustGage({
    id: "gauge_humid",
    value: 0,
    min: 0,
    max: 100,
    title: "Humidity",
    label: "%",
    gaugeWidthScale: 0.6,
    levelColors: ["#00ff00", "#ffcc00", "#ff0000"],
    counter: true
});

let gaugeMoisture = new JustGage({
    id: "gauge_moisture",
    value: 0,
    min: 0,
    max: 100,
    title: "Soil Moisture",
    label: "%",
    gaugeWidthScale: 0.6,
    levelColors: ["#ff0000", "#ffcc00", "#00ff00"],
    counter: true
});

function updateUI(data) {
    // Handle potential NaN values
    const temp = isNaN(data.temp) ? 0 : data.temp;
    const humid = isNaN(data.humid) ? 0 : data.humid;
    const moisture = isNaN(data.moisture) ? 0 : data.moisture;

    gaugeTemp.refresh(temp);
    gaugeHumid.refresh(humid);
    gaugeMoisture.refresh(moisture);
    
    document.getElementById("pir").innerText = data.pir ? "ACTIVE" : "INACTIVE";
    document.getElementById("pir").style.color = data.pir ? "#dc3545" : "#28a745";
    document.getElementById("relay").innerText = data.relay ? "ON" : "OFF";
    document.getElementById("relay").style.color = data.relay ? "#28a745" : "#dc3545";
    document.getElementById("manual").innerText = data.manual ? "MANUAL" : "AUTO";
    document.getElementById("manual").style.color = data.manual ? "#ffc107" : "#007bff";

    // Check for motion detection
    if (data.pir && !lastPirState) {
        sendEmailAlert();
    }
    lastPirState = data.pir;
}

async function controlPump(state) {
    const statusElement = document.getElementById("status");
    statusElement.className = "status";
    
    try {
        const response = await fetch(`http://${ESP_IP}/control?state=${state}`, {
            timeout: 5000 // 5 second timeout
        });
        
        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        
        const result = await response.text();
        console.log("Control result:", result);
        
        statusElement.textContent = `Success: ${result}`;
        statusElement.classList.add("success");
        
        // Refresh data after 1 second to get updated state
        setTimeout(fetchData, 1000);
    } catch (error) {
        console.error("Error:", error);
        statusElement.textContent = `Error: ${error.message}`;
        statusElement.classList.add("error");
    }
}

async function fetchData() {
    const statusElement = document.getElementById("status");
    
    try {
        const response = await fetch(`http://${ESP_IP}/data`, {
            timeout: 5000 // 5 second timeout
        });
        
        if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
        
        const text = await response.text();
        // Handle potential "nan" values in response
        const sanitizedText = text.replace(/nan/g, '0')
                                 .replace(/Infinity/g, '0')
                                 .replace(/-?1\.#IND/g, '0');
        
        const data = JSON.parse(sanitizedText);
        connectionRetries = 0; // Reset retry counter on success
        updateUI(data);
        
        statusElement.textContent = "Connected";
        statusElement.className = "status success";
        
    } catch (error) {
        console.error("Error fetching data:", error);
        connectionRetries++;
        
        if (connectionRetries <= MAX_RETRIES) {
            statusElement.textContent = `Retrying... (${connectionRetries}/${MAX_RETRIES})`;
            statusElement.className = "status warning";
        } else {
            statusElement.textContent = `Connection Error: ${error.message}`;
            statusElement.className = "status error";
        }
        
        // Exponential backoff for retries
        const retryDelay = Math.min(1000 * Math.pow(2, connectionRetries), 30000);
        setTimeout(fetchData, retryDelay);
    }
}

function sendEmailAlert() {
    const params = {
        time: new Date().toLocaleString(),
        to_email: "your_email@example.com", // Replace with your email
        subject: "Motion Detected in Plant System!",
        message: "The PIR sensor detected motion near your plants!",
        system_ip: ESP_IP
    };

    emailjs.send("service_rs7123p", "template_empcsua", params)
        .then((response) => {
            console.log("Email sent successfully!", response.status, response.text);
        }, (error) => {
            console.error("Failed to send email:", error);
            // Optionally retry email sending
            setTimeout(sendEmailAlert, 5000);
        });
}

// Start data refresh when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    fetchData();
    dataRefreshInterval = setInterval(fetchData, 2000);
});

// Handle page visibility changes
document.addEventListener('visibilitychange', function() {
    if (document.hidden) {
        clearInterval(dataRefreshInterval);
    } else {
        fetchData();
        dataRefreshInterval = setInterval(fetchData, 2000);
    }
});
const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const mqtt = require('mqtt');

const app = express();
const server = http.createServer(app);
const io = socketIo(server);

app.use(express.static('public'));

// Conexión al Broker MQTT
const mqttClient = mqtt.connect('mqtt://localhost:1883');

mqttClient.on('connect', () => {
    console.log('Node.js conectado a MQTT');
    mqttClient.subscribe('test/gps'); // Suscribirse al tópico nuevo
});

mqttClient.on('message', (topic, message) => {
    // El mensaje viene como Buffer, lo pasamos a String
    const mensajeString = message.toString();
    console.log(`Recibido: ${mensajeString}`);

    try {
        // Intentamos entender el JSON que envía el ESP32
        const datosJSON = JSON.parse(mensajeString);
        
        // Enviamos el objeto limpio a la web
        io.emit('datos_sensor', datosJSON);
    } catch (e) {
        console.error("Error al leer JSON del ESP32:", e);
    }
});

server.listen(3000, () => {
    console.log('Servidor Web en http://localhost:3000');
});
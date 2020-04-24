# particlePhoton

Small smarthome project for my office and also a neighbor. Controls windows or rollers motors with shelly.cloud WLAN relays over a Particle.io Photon device. The photon tracks room temperature with a DHT22 sensor itself and receives meteo data over its API.

Start: Serve index.html from the public folder for a static only usecase or run the express.js app [host:8080/windows] if you do not want do bother users with username/password.

Particle.io Photon Device: https://docs.particle.io/photon/

Shelly Devices:https://shelly.cloud/shelly-25-wifi-smart-relay-roller-shutter-home-automation/

Meteo Data for Switzerland: https://github.com/marcelstoer/open-data-smn

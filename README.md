# Window automation with particlePhoton

Small smarthome project for my office and also a neighbor. Controls windows or rollers motors with shelly.cloud WLAN relays over a Particle.io Photon device. The photon tracks room temperature with a DHT22 sensor itself and receives meteo data over its API. Make sure you give the shellys a fixed IP address so you can code that address to the particle device.

Start: Serve index.html from the public folder for a static only usecase or run the express.js app [host:8080/windows] if you do not want do bother users with username/password on client side.

Particle.io Photon Device: https://docs.particle.io/photon/ (works fine with dynamic IP address as long as it is part of the same WLAN. It only has to know the fixed IP of the shellys)

Shelly Devices:https://shelly.cloud/shelly-25-wifi-smart-relay-roller-shutter-home-automation/

Meteo Data for Switzerland: https://github.com/marcelstoer/open-data-smn is sent as json to the particle cloud (Particle.publish) and for my setup runs on a raspberry PI. All particle devices that belong to a certain user can subscribe to events like the meteodata above and the particle cloud will send those events immediatly to all devices.

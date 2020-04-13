var Particle = require('particle-api-js');
var particle = new Particle();
var token;

particle.login({username: 'guido.schnider@gmail.com', password: 'INl16ajcBpkb'}).then(
  function(data) {
    token = data.body.access_token;

    var data = {sunshine:10, outsidetemperature: 25, status:"ok"}

    var publishEventPr = particle.publishEvent({ name: 'meteodata', data:
    JSON.stringify(data), auth: token });

    publishEventPr.then(
    function(data) {
        if (data.body.ok) { console.log("Event published succesfully") }
    },
    function(err) {
        console.log("Failed to publish event: " + err)
    }
    );

  },
  function (err) {
    console.log('Could not log in.', err);
  }
);


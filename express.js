var Particle = require('particle-api-js');
var particle = new Particle();
var express = require('express');
var app = express();
var handlebars = require('express-handlebars');

app.engine('handlebars', handlebars());
app.set('view engine', 'handlebars');
app.use(express.static('public'));

var username = "guido.schnider@gmail.com";
var password = "";
var token;

doLogin();

app.get('/windows', function (req, res) {
  res.locals.token = token;
  res.render('main');
});

app.route('/B14-2-21').get(function(req,res)
{
    res.send("Content for B14...");
});

app.listen(8080);


function doLogin() {
  particle.login({ username: username, password: password }).then(
    function (data) {
      token = data.body.access_token;
      console.log("Login successful, ready to call API.");
    },
    function (err) {
      console.log('Could not log in.', err);
    }
  );
}


'use strict';
const NodeHelper = require('node_helper');

const {PythonShell} = require('python-shell');
var pythonStarted = false

module.exports = NodeHelper.create({

 	python_start: function () {
		const self = this;
//    		const pyshell = new PythonShell('modules/' + this.name + '/camera_publisher/image_webcam_broadcaster.py', { mode: 'json', args: [JSON.stringify(this.config)]});
    		self.pyshell = new PythonShell('modules/' + this.name + '/camera_publisher/image_realsense_broadcaster_cplusplus.py', {pythonPath: 'python', mode: 'json', args: [JSON.stringify(this.config)]});

    		self.pyshell.on('message', function (message) {
			try {
				//console.log("[MSG " + self.name + "] " + message);
      				if (message.hasOwnProperty('status')){
      					console.log("[" + self.name + "] " + message.status);
      				}
			}
			catch(err) {
				console.log("[" + self.name + "] " + err );
			}
    		});
  },

  // Subclass socketNotificationReceived received.
  socketNotificationReceived: function(notification, payload) {
    if(notification === 'CONFIG') {
      this.config = payload
      if(!pythonStarted) {
        pythonStarted = true;
        this.python_start();
	this.hidden = true;
        };
    };
  },

  stop: function() {
		const self = this;
		self.pyshell.childProcess.kill('SIGINT');
		self.pyshell.childProcess.kill('SIGKILL');
		self.pyshell.end(function (err) {
           	if (err){
        		//throw err;
    		};
    		console.log('finished');
		});
		
	}

});

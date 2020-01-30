'use strict';
const NodeHelper = require('node_helper');
const { spawn } = require('child_process');

const {PythonShell} = require('python-shell');
var pythonStarted = false

module.exports = NodeHelper.create({

 	python_start: function () {
		const self = this;

			self.objectDet = spawn('modules/' + this.name + '/camera_publisher/realsense_cplusplus/build/camera_publisher',[self.config.image_width, self.config.image_height]);

			self.objectDet.stdout.on('data', (data) => {
				try{
					var parsed_message = JSON.parse(`${data}`)
	
					if (parsed_message.hasOwnProperty('CAMERA_FPS')){
						//console.log("[" + self.name + "] object detection fps: " + JSON.stringify(parsed_message));
						self.sendSocketNotification('CAMERA_FPS', parsed_message.CAMERA_FPS);
					}else if (parsed_message.hasOwnProperty('STATUS')){
						console.log("[" + self.name + "] status received: " + JSON.stringify(parsed_message));
					}
				}
				catch(err) {	
				//console.log(err)
				}
  				//console.log(`stdout: ${data}`);
			});	
		
    		/*self.pyshell = new PythonShell('modules/' + this.name + '/camera_publisher/image_realsense_broadcaster_cplusplus.py', {pythonPath: 'python3', args: [JSON.stringify(this.config)]});
    		//self.pyshell = new PythonShell('modules/' + this.name + '/camera_publisher/image_realsense_broadcaster_cplusplus.py', {pythonPath: 'python', mode: 'json', args: [JSON.stringify(this.config)]});

    		self.pyshell.on('message', function (message) {
			try {
				var parsed_message = JSON.parse(message)
				//console.log("[MSG " + self.name + "] " + message);
      				if (parsed_message.hasOwnProperty('status')){
      					console.log("[" + self.name + "] " + parsed_message.status);
      				}
			}
			catch(err) {
				console.log("[" + self.name + "] " + err );
			}
    		}); */
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

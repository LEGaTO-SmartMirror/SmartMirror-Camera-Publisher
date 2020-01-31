/**
 * @file smartmirror-camera-publisher.js
 *
 * @author nkucza
 * @license MIT
 *
 * @see  https://github.com/NKucza/smartmirror-camera-publisher
 */


Module.register('SmartMirror-Camera-Publisher',{

	defaults: {
		//Module set used for strangers and if no user is detected
		defaultClass: "default",
		//Set of modules which should be shown for every user
		everyoneClass: "everyone",
		//Set of modules which should be shown for every user
		forAllClass: "forall",
		rotation: 90.0,
		image_width: 1920, 
		image_height: 1080

	},

	start: function() {
		this.sendSocketNotification('CONFIG', this.config);
		Log.info('Starting module: ' + this.name);
	},

	notificationReceived: function(notification, payload) {
       	
    },

	socketNotificationReceived: function(notification, payload) {
		if (notification === 'CAMERA_FPS') {
			this.sendNotification('CAMERA_FPS', payload);
		};
	}

});

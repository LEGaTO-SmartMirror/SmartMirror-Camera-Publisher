/**
 * @file smartmirror-camera-publisher.js
 *
 * @author nkucza
 * @license MIT
 *
 * @see  https://github.com/NKucza/smartmirror-camera-publisher
 */


Module.register('smartmirror-camera-publisher',{

	defaults: {
		//Module set used for strangers and if no user is detected
		defaultClass: "default",
		//Set of modules which should be shown for every user
		everyoneClass: "everyone",
		//Set of modules which should be shown for every user
		forAllClass: "forall",
	},

	start: function() {
		this.sendSocketNotification('CONFIG', this.config);
		Log.info('Starting module: ' + this.name);
	}

});

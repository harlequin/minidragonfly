var show = (new function($) {
	'use strict';
	
	var show_table;
		
	this.init = function(options) {
		
		show_table = $('#show-table');
		
		show_table.fasttable({
			headerCheck: $('#show-table > thead > tr:first-child'),
			pageSize: 200,
			maxPages: 5,
			fillFieldsCallback: fill_callback,			
		});
		
		show_table.on('click', 'a', editClick);
		
	}
	
	function editClick(e) {
		console.log("edit");
		e.preventDefault();
		ConfirmDialog.showModal('edit-tv-episode-dialog', performEdit);
	}
	
	function performEdit() {
		console.lof("performEdit");
	}
	
		
	this.update = function(id) {
		console.log(id);
		RPC.call("episodes", [ id ], loaded);		
	}
	
	function loaded (items) {
		
		var data = [];
		
		for( var i = 0; i < items.length; i++) {
			var item = {
				id: items[i].id,
				season: items[i].season,
				episode: items[i].episode,
				title: items[i].title,
				status: items[i].status,
			};
			data.push(item)
		}
		
		show_table.fasttable('update', data)
	}
	
	function fill_callback(item) {
		var status = '<span class="label label-default label-'+ Frontend.states[item.status].toLowerCase() +'">'+ Frontend.states[item.status] +'</span>';
		
		item.fields = [ item.season, item.episode, '<a href="" item="' + item.id + '">' + item.title + '</a>', status ];		
	}	
	
} (jQuery));
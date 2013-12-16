var tvshows = (new function($) {
	'use strict';
	
	var tvshows_table;
		
	this.init = function(options) {
		
		tvshows_table = $('#tvshows-table');		
		tvshows_table.fasttable({
			filterInput: $('#show-search'),
			filterCaseSensitive: false,
			headerCheck: $('#tvshows-table > thead > tr:first-child'),
			pageSize: 100,
			maxPages: 5,
			fillFieldsCallback: fill_callback,
			updateInfoCallback: update_info_callback,
		});
		
		$('#show-search').keyup(function(){
			if( Frontend.shows_total != 0) {
				$('#tvshows-tab-button').click();
			}
		});
		
		
		
		tvshows_table.on('click', 'a', editClick);
		
	}
	
	function editClick(e) {
		e.preventDefault();		
		
		if( $(this).attr('mode') == 'search') {
			RPC.call("search", [$(this).attr('showid')]);
		} else if ($(this).attr('mode') == 'delete') {
			var id = $(this).attr('showid');
			
			ConfirmDialog.showModal('delete-tv-show-dialog', function(){				
				RPC.call("delete", [id]);
				tvshows.update();
			});
			
			
		} else {		
			$('#tvshow-info-tab-button').click();
			show.update( $(this).attr('showid') );
		}		
	}
		
	
	this.update = function() {
		RPC.call("shows", [], loaded)	
	}
	
	function update_info_callback(data) {
		
		if(data.filter == true && data.available == 0) {
			$('#show-search-button').removeAttr('disabled');
		} else {
			$('#show-search-button').attr('disabled', 'disabled');
		}		
	}
	
	function loaded (items) {
		
		var data = [];
		
		for( var i = 0; i < items.length; i++) {
			var item = {
				id: items[i].id,
				name: items[i].name,
				search: items[i].name,
			};
			data.push(item)
		}
		
		tvshows_table.fasttable('update', data)
	}
	
	function fill_callback(item) {
		item.fields = [ create_action_item(item.id), '<a href="" showid="' + item.id + '">' + item.name + '</a>', item.skipped ];		
	}
	
	function create_action_item(id) {
		return '<div class="btn-group">' +
				'<a type="button" class="btn btn-default" mode="search" showid="' + id + '"><span class="glyphicon glyphicon-search"></span></a>' +
				'<a type="button" class="btn btn-default" mode="delete" showid="' + id + '"><span class="glyphicon glyphicon-trash"></span></a>' +
				'</div>'
	}
	
	
} (jQuery));
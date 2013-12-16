/*** CONFIRMATION DIALOG *****************************************************/

var ConfirmDialog = (new function($)
{
	'use strict';

	// Controls
	var $ConfirmDialog;
	
	// State
	var actionCallback;
	
	this.init = function()
	{
		$ConfirmDialog = $('#ConfirmDialog');
		$ConfirmDialog.on('hidden', hidden);
		$('#ConfirmDialog_OK').click(click);
	}

	this.showModal = function(id, callback)	{
				
	
		$('#ConfirmDialog_Title').html($('#' + id + '-title').html());
		$('#ConfirmDialog_Text').html($('#' + id + '-text').html());
		$('#ConfirmDialog_OK').html($('#' + id + '-ok').html());
		actionCallback = callback;
				
		$ConfirmDialog.modal({backdrop: 'static'});
		
		
		// avoid showing multiple backdrops when the modal is shown from other modal
		var backdrops = $('.modal-backdrop');
		if (backdrops.length > 1)
		{
			backdrops.last().remove();
		}
	}

	function hidden()
	{
		// confirm dialog copies data from other nodes
		// the copied DOM nodes must be destroyed
		$('#ConfirmDialog_Title').empty();
		$('#ConfirmDialog_Text').empty();
		$('#ConfirmDialog_OK').empty();
	}

	function click(event)
	{
		event.preventDefault(); // avoid scrolling
		if(actionCallback)
			actionCallback();
		$ConfirmDialog.modal('hide');
	}
}(jQuery));



var Home = (new function($) {
	'use strict';
	this.init = function() {
		
	}
}(jQuery));




var Frontend = (new function($) {
	'use strict';
	
	this.shows_total = 0;
	this.states = new Array();
	
	this.init = function() {
		
		tvshows.init();
		tvshows.update();
		
		show.init();
		
		ConfirmDialog.init();
	
		RPC.call('stats', [], load_stats);
		RPC.call('ui.data', [], load_ui_data);
		
		RPC.call('news',[], loaded);		
	}
	
	
	function loaded(items) {
		console.log("news loaded");
		console.log(items);
	}
	
	
	function news_loaded(items) {
		console.log(items);
		var data = [];
		for( var i = 0; i < items.length; i++) {

			var item =	{
				id: items[i].id,
				comment: items[i].comment,
			};
			data.push("<a showid='" + item.id + "' href='#tvshow-info-tab' data-toggle='tab' onClick='javascript:show.update(" + item.id + ");' class='thumbnail' ><img data-src='holder.js/360x270' src='cache/tvthumb/" + item.id + ".png' alt=''  /></a>");
			
			//$('#tvshow-info-tab-button').click();
			//show.update( $(this).attr('showid') );
		}
		$('#recent_actions').append(data.join(''));
	}
	
	function load_ui_data(items) {
		console.log(items)
		
		/* episode states */
		$('#tv_show_search_status').empty();
		for(var i = 0; i < items.episode_states.length; i++) {	
			
			Frontend.states[items.episode_states[i].value] = items.episode_states[i].name;
			
			$('#tv_show_search_status').append(
				$("<option></option>").text(items.episode_states[i].name).val(items.episode_states[i].value)				
			);
		}
		
		/* quality preset */
		
		$('#add-show-quality-preset').empty();
		for(var i = 0; i < items.quality_presets.length; i++) {	
			$('#add-show-quality-preset').append(
				$("<option></option>").text(items.quality_presets[i].name).val(items.quality_presets[i].value)
			);
		}
		//tv_show_search_status
	}
	
	
	
	function load_stats(items){
		console.log(items)
		
		if( items.shows_total == 0 ) {
			$('#tvshows-tab-button').hide();
		} else {
			$('#tvshows-tab-button').show();
		}		
		Frontend.shows_total = items.shows_total;
	}
	
	
	this.manualPostProcess = function() {
		ConfirmDialog.showModal('manual-post-processing-dialog', function(){
			console.log($('#manual-post-processing-dialog-location').val());
			RPC.call("process", [ $('#manual-post-processing-dialog-location').val() ], 				
				function (){
					console.log("success");
				},				
				function (){
					console.log("error");
				}			
			);				
		});
	}
	
	
	
	
	
	this.shutdownConfirm = function() {
		ConfirmDialog.showModal('shutdown-confirm-dialog', shutdown);
	}
	
	function shutdown() {
		RPC.call("shutdown", []);
	}
		
	this.searchTvdb = function() {			
		RPC.call("searchtvdb", [ $('#show-search').val() ], search_callback, search_error_callback );
		
		ConfirmDialog.showModal('add-tv-show-dialog', add_tv_show);
	}
	
	
	function search_callback(items) {
		var i;		
		
		$('#add-tv-show-loader').hide();
		$('#add-tv-show-dialog-content').show();
		$('#tvshow_search_results')
			.find('option')
			.remove()
			.end();
		
		for( i = 0; i< items.length; i++) {
			console.log(items[i]);
			$('#tvshow_search_results').append($("<option />").val(items[i].id).text(items[i].name));
		}		
		
		
		
	}
	
	
	function add_tv_show() {
		//<select id="tv_show_search_status" class="form-control"></select>
		//<select id="add-show-quality-preset" class="form-control"></select>
		RPC.call('show.addnew', [$('#tvshow_search_results').val(), $('#search_language').val(), $('#tv_show_search_status').val(), $('#add-show-quality-preset').val()]);		
		tvshows.update();
	}
	
	
	function search_error_callback(error) {
		console.log(error);
	}
	
} (jQuery));



$(document).ready(function()
{
	Frontend.init();	
});
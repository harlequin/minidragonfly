


	$('#OpenSearchDialog').on('click', function(ev) {
		$('#SearchShowDialog').modal().show();
	});

	$('#shutdown').on('click', function (ev) {
		RPC.call("shutdown");
	});



	$('#scan_all_button').on('click', function(ev) {
		console.log("Scan all wanted episodes");
		$.getJSON("/json?method=search_all",{
		
		});
	});
	
	$('#delete_tv_show').on('click', 'a', function(ev) {	
		console.log("Hallo");
		//console.log($(this));
		//console.log("Delete tv show " + $(this).attr('tv_show_id'));
	});

	$('#search_show_button').on('click', function(ev){
		console.log($('#search_status').val());
		
		$.getJSON("/json?method=search_series_by_name",{
				q: $('#search_show_value').val(),
				language: $('#search_language').val(),
				status: $('#search_status').val()
            }, function(data) {

				$('#search_results')
					.find('option')
					.remove()
					.end();

			
				if(data.series.length === 0) {
					$('#myModal').modal('show');
				} else {
					var arr = [], i=data.series.length;
					while(i--){						
						$('#search_results').append($("<option />").val(data.series[i].id).text(data.series[i].name));					
					}		
				}
				
			});					
	});
		
	$('#add_show_button').on('click', function(ev){
		console.log("add clicked");
		console.log($('#search_results').val());
		console.log($('#search_results :selected').text());
		
		$.getJSON("/json?method=add_series_by_id",{
			q: $('#search_results').val(),
			language: $('#search_language').val(),
			status: $('#search_status').val(),
			quality: $('#search_quality').val(),
			match: $('#search_operator').val(),
			location: $('#search_location').val()
            }, function(data) {			
				if(data.result == false) {
					$('#error-topic').text("Add TV Show ...");
					$('#error-body-message').text(data.message);
					$('#myModal').modal('show');
				} else {
					Series.update();
					News.refresh();
				}
			});		
	});


var Settings = (new function($) {
	'use strict';
	
	
	var quality_names = new Array();
	
	this.getQualityNames = function() {
		return quality_names;
	}
	
	
	this.init = function() {
		RPC.call('config', ['quality-presets'], loaded);
		RPC.call('config', ['quality-names'], loaded2);
	
		$('#search_quality').click(function() {			
			if(this.options[this.selectedIndex].value == -1) {
				$('#custom_search_quality').show();
			} else {
				$('#custom_search_quality').hide();
			}			
		});
		
	}
	
	function loaded2(items) {
		$('#custom_search_quality').empty();		
		var i = 0;
		for(i = 0; i < items.length; i++) {
			$('#custom_search_quality').append(
				$("<option></option>").text(items[i].name).val(items[i].value)
			);		
			quality_names[items[i].value] = items[i].name;			
		}		
	}
	
	function loaded(items) {
		$('#search_quality').empty();
		
		$('#search_quality').append(
			$("<option></option>").text("Custom").val(-1)
		);		
		
		var i = 0;
		for(i = 0; i < items.length; i++) {
			$('#search_quality').append(
				$("<option></option>").text(items[i].name).val(items[i].value)
			);

			
		}		
	}
}(jQuery));


var show_details = (new function($) {
	'use strict';
	
	this.init = function() {
		$('#EpisodesTable').fasttable(
			{
				//filterInput: $('#HistoryTable_filter'),
				//filterClearButton: $("#HistoryTable_clearfilter"),
				//pagerContainer: $('#HistoryTable_pager'),
				//infoContainer: $('#HistoryTable_info'),
				headerCheck: $('#EpisodesTable > thead > tr:first-child'),
				filterCaseSensitive: false,
				pageSize: 200, //recordsPerPage,
				maxPages: 5,
				//pageDots: !UISettings.miniTheme,
				fillFieldsCallback: fillFieldsCallback,
				//renderCellCallback: renderCellCallback,
				//updateInfoCallback: updateInfo
			});
	}	
	
	this.update = function(id)
	{
		$('#tv').removeClass("active");
	
		var data = [];
		
		$.getJSON("/json?method=list_episodes",{
				q: id
            }, function(result) {		
            var arr = [], i=result.series.length;
            while(i--){
				var item = {
					id: result.series[i].id,
					title: result.series[i].title,
					season: result.series[i].season,
					episode: result.series[i].episode,
					quality: result.series[i].quality,
					status: result.series[i].status,
					nzb: result.series[i].nzb,
					language: result.series[i].language,
				};
				data.push(item);	
            }				
			$('#EpisodesTable').fasttable('update', data);
        });
	}
	
	function fillFieldsCallback(item)
	{	
		var status;
		
		if(item.status == 0) {
			status = '<span class="label label-warning">Skipped</span>'
		} else if ( item.status == 1) {
			status = '<span class="label label-important">Wanted</span>'
		} else if ( item.status == 2) {
			status = '<span class="label label-success">Archived</span>'
		} else if ( item.status == 3) {
			status = '<span class="label label-info">Ignored</span>'
		} else if ( item.status == 4) {
			status = '<span class="label label-info">Snatched</span>'		
		} else {
			status = '<span class="label label-importand">Unknown [' + item.status + ']</span>'
		}
		
		var quality = "";
		if( item.quality != "" )
			quality = '<span class="label label-inverse">' + Settings.getQualityNames()[item.quality] + '</span>'
		
		item.fields = [ '<div class="flag flag-' + item.language + '"></div>' , item.season, item.episode, item.title, quality , status, item.nzb];
	}
	
	this.redraw = function() {
		//console.log("redraw");
	}
}(jQuery));


var SearchEpisode = (new function($) {

	'use strict';

	this.search_by_details = function ( id ) {
	
	}
	
}(jQuery));



var News = (new function($) {
	'use strict';

	this.refresh = function() {
		
		
		
		
		var data = [];
		$('#news').empty();//.innerHTML = "";
		$.getJSON("/json?method=get_latest_news", function(result) { 			                    
			var arr = [], i = result.feeds.length;
			
			while(i--){
				//console.log("ITEM");
				var item =
				{
					id: result.feeds[i].id,
					comment: result.feeds[i].comment,
					overview: result.feeds[i].overview,
				};
				data.push("<div class='item " + ( data.length == 0 ? "active" : "" ) + "' style='background-color:black' ><div style='-webkit-mask-image: -webkit-linear-gradient(left, rgba(0,0,0,1), rgba(0,0,0,0));'><img src='cache/hdclearart/" + item.id + ".png' style=' height: 562px' alt=''  /></div><div class='carousel-caption'><p>"+  item.overview  +"</p></div></div>");		
			}		

			if(data.length == 0) {
				data.push("<div class='item active' style='background-color:black' ><div style='-webkit-mask-image: -webkit-linear-gradient(left, rgba(0,0,0,1), rgba(0,0,0,0));'><img src='front.png' style=' height: 562px' alt=''  /></div><div class='carousel-caption'><p>minidragonfly</p></div></div>");		
			}
			
			$('#news').append(data.join(''));			
		});
	}
	
}(jQuery));


/*** Series Main Page ***********************************************************/

var Series = (new function($)
{
	'use strict';
	
	this.init = function(options)
	{
		//console.log("Series tab init");
		//$('#SeriesTable');
		
		$('#SeriesTable').fasttable(
			{
				//filterInput: $('#HistoryTable_filter'),
				//filterClearButton: $("#HistoryTable_clearfilter"),
				pagerContainer: $('#DownloadsTable_pager'),
				infoContainer: $('#ShowTable_info'),
				headerCheck: $('#SeriesTable > thead > tr:first-child'),
				filterCaseSensitive: false,
				pageSize: 50,
				maxPages: 5,//UISettings.miniTheme ? 1 : 5,
				pageDots: true,// !UISettings.miniTheme,
				fillFieldsCallback: fillFieldsCallback,
				renderCellCallback: renderCellCallback,
				//updateInfoCallback: updateInfo
			});
		
		function show_delete() {
		}
		
		$('#SeriesTable').on('click', 'a', function(event){
			var id = $(this).attr('tv_show_id');
			
			if($(this).context.id == 'scan_tv_show') {
				console.log('search for ' + id);
				RPC.call('search', [id], '', '');	
			}
			
			if($(this).context.id == 'delete_tv_show') {
				console.log("do you want to delete show " + id);
				
				/*				
				$('#QuestionDialog').modal('show');
				$('#delete_show_button').unbind('click');				
				$('#delete_show_button').click(function() {
					$('#QuestionDialog').modal('hide');
				*/
					RPC.call('delete', [id], '', '');	

					/*
					$.getJSON("/json?method=tv_show_delete",
					{
						q: id
					}, function(data) {
						Series.update();
					});				
					*/
				//	return true;
				//});				
			}		
		});
	}
	
	this.update = function()
	{
		var data = [];
		//console.log("update");
		$.getJSON("/json?method=list_series", function(result) { 			
                     
            var arr = [], i=result.series.length;
            while(i--){
				var item =
				{
					id: result.series[i].id,
					name: result.series[i].name,
					languages: [],
					skipped : result.series[i].skipped,
					wanted : result.series[i].wanted,
					archived : result.series[i].archived,
					ignored : result.series[i].ignored,
					snatched : result.series[i].snatched,
					total: result.series[i].total,
				};
				
				var j = result.series[i].languages.length;
				//console.log(j);
				while(j--) {
					item.languages.push(  result.series[i].languages[j] );
				}
				
				data.push(item);	
				//console.log(item);
				//console.log("item added");				
            }				
			$('#SeriesTable').fasttable('update', data);
        });
	}
	
	function fillFieldsCallback(item)
	{	
		var languages = "";
		var j = item.languages.length;
		while(j--) {
			languages += '<div class="flag flag-' + item.languages[j] + '"></div>'			
		}
		//console.log(languages);
	
		var skipped_bar = '<div class="bar bar-danger" style="width: ' + (item.skipped / item.total *100)+ '%;"></div>';
		var wanted_bar = '<div class="bar bar-important" style="width: ' + (item.wanted / item.total *100)+ '%;"></div>';
		var archived_bar = '<div class="bar bar-success" style="width: ' + (item.archived / item.total *100)+ '%;"></div>';
		var ignored_bar = '<div class="bar bar-info" style="width: ' + (item.ignored / item.total *100)+ '%;"></div>';
		var snatched_bar = '<div class="bar bar-warning" style="width: ' + (item.snatched / item.total *100)+ '%;"></div>';
	
		item.fields = [	'<div class="btn-group">' +
						'<a id="scan_tv_show"   class="btn btn-small" href="#" tv_show_id="' + item.id + '"><i class="icon-search"></i></a>' +
						'<a id="edit_tv_show"   class="btn btn-small" href="#" tv_show_id="' + item.id + '"><i class="icon-pencil"></i></a>' +
						'<a id="delete_tv_show" class="btn btn-small" href="#" tv_show_id="' + item.id + '"><i class="icon-trash"></i></a></div>'
						, languages
						, '<a showid="' + item.id + '" href="#show_details" data-toggle="tab" onClick="javascript:show_details.update(' + item.id + ');" >' + item.name + '</a>',
						
						'<div class="progress">' + skipped_bar + wanted_bar + archived_bar + ignored_bar + snatched_bar +'</div>',
						
						item.skipped,
						item.wanted ,
						item.archived ,
						item.ignored ,
						item.snatched 
						
						
						
						];
	}
	
	function renderCellCallback(cell, index, item)
	{
		if (index === 10)
		{
			cell.className = 'text-center';
		}
	}
	
	this.recordsPerPageChange = function()
	{
		var val = 5;		
		$('#SeriesTable').fasttable('setPageSize', val);
	}
	
	
}(jQuery));

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

	this.showModal = function(id, callback)
	{
		$('#ConfirmDialog_Title').html($('#' + id + '_Title').html());
		$('#ConfirmDialog_Text').html($('#' + id + '_Text').html());
		$('#ConfirmDialog_OK').html($('#' + id + '_OK').html());
		Util.centerDialog($ConfirmDialog, true);
		actionCallback = callback;
		$ConfirmDialog.modal();
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
		actionCallback();
		$ConfirmDialog.modal('hide');
	}
}(jQuery));



var ManualPostProcessDialog = (new function($)
{
	'use strict'
	
	var $postprocess_dialog;

	this.init = function() {
		$postprocess_dialog = $('#postprocess_dialog');
	}
	
	this.showModal = function(id, callback) {
	
	}
	
	function hidden() {
	
	}
	
	function click(event) {
		event.preventDefault();
		actionCallback();
		$postprocess_dialog.modal('hide');
	}
	
	this.process = function() {		
		RPC.call("process", [$('#postprocess_directory').val()])		
		//$postprocess_dialog.modal('hide');
	}
	
}(jQuery));





/*** FRONTEND MAIN PAGE ***********************************************************/

var Frontend = (new function($)
{
	'use strict';
	
	this.init = function()
	{		
		Settings.init();
		Series.init();
		Series.update();
		show_details.init();
		News.refresh();
		
		
		ManualPostProcessDialog.init();
	}	
});

/*** START WEB-APPLICATION ***********************************************************/

$(document).ready(function()
{
	Frontend.init(); 
});



/*** REMOTE PROCEDURE CALLS VIA JSON-RPC *************************************************/

var RPC = (new function($)
{
	'use strict';
	
	// Properties
	this.rpcUrl = "/jsonrpc";
	//this.defaultFailureCallback;
	this.connectErrorMessage = 'Cannot establish connection';

	this.call = function(method, params, completed_callback, failure_callback)
	{
		var _this = this;
		
		var request = JSON.stringify({nocache: new Date().getTime(), method: method, params: params});
		var xhr = new XMLHttpRequest();

		xhr.open('post', this.rpcUrl);

		// Example for cross-domain access:
		//xhr.open('post', 'http://localhost:6789/jsonrpc');
		//xhr.withCredentials = 'true';
		//xhr.setRequestHeader('Authorization', 'Basic ' + window.btoa('myusername:mypassword'));

		xhr.onreadystatechange = function()
		{
			if (xhr.readyState === 4)
			{
					var res = 'Unknown error';
					var result;
					if (xhr.status === 200)
					{
						//console.log("e");
					
						if (xhr.responseText != '')
						{
							try
							{	
								//console.log(xhr.responseText);							
								result = JSON.parse(xhr.responseText);
								//console.log(result);
							}
							catch (e)
							{
								console.log(xhr.responseText);
								console.log(e);
								res = e;								
							}
							if (result)
							{
								
								//console.log(result.error)
							
								if (result.error == null)
								{
								
									res = result.data;
									if(completed_callback)
										completed_callback(res, result.version, result.hash);									
									return;
								}
								else
								{
									res = result.error.message + '<br><br>Request: ' + request;
									//console.log(res);
								}
							}
						}
						else
						{
							res = 'No response received.';
							console.log(res);
						}
					}
					else if (xhr.status === 0)
					{
						res = _this.connectErrorMessage;
						
						$('#error-dialog-title').text("RPC error");
						$('#error-dialog-text').text(res);
						$('#error-dialog').modal('show');
						
						
						console.log(res);
					}
					else
					{
						res = 'Invalid Status: ' + xhr.status;
						console.log(res);
					}
					
					
					

					if (failure_callback)
					{
						failure_callback(res, result);
					}
					else
					{
						//_this.defaultFailureCallback(res, result);
					}
			}
		};
		xhr.send(request);
	}
	}(jQuery));
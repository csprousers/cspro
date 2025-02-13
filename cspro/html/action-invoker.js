class CSProActionInvoker{static $Impl={createMessage:function(aiThis,action,args,requestId){return JSON.stringify({accessToken:aiThis.accessToken,action:action,arguments:function(args){if(args===undefined||typeof args==="string"){return args;}
else{return JSON.stringify(args);}}(args),requestId:requestId,url:window.location.href});},processResponse:function(responseJson){const response=JSON.parse(responseJson);if(response.type==="exception"){throw new Error(response.value);}
else{return response.value;}},usingWindows:function(){return(typeof AndroidActionInvoker==="undefined");},run:function(aiThis,action,args){const message=this.createMessage(aiThis,action,args);if(this.usingWindows()){const hostSync=window.chrome.webview.hostObjects.sync.cspro;hostSync.setHostProperty("ActionInvoker",message);var response=hostSync.getHostProperty("ActionInvoker");}
else{var response=AndroidActionInvoker.run(message);}
return this.processResponse(response);},nextRequestId:1,callbacks:{},runAsync:function(aiThis,action,args){const requestId=this.nextRequestId++;const message=this.createMessage(aiThis,action,args,requestId);return new Promise((resolve,reject)=>{this.callbacks[requestId]={resolve:resolve,reject:reject};if(this.usingWindows()){const hostSync=window.chrome.webview.hostObjects.sync.cspro;hostSync.setHostProperty("ActionInvokerAsync",message);}
else{AndroidActionInvoker.runAsync(message);}});},processAsyncResponse:function(requestId,responseJson){const requestCallback=this.callbacks[requestId];delete this.callbacks[requestId];try{requestCallback.resolve(this.processResponse(responseJson));}
catch(e){requestCallback.reject(e);}}}
constructor(accessToken){this.accessToken=accessToken;}
getWindowForEventListener(){if(CSProActionInvoker.$Impl.usingWindows()){return window.chrome.webview;}
else{return window;}}

execute(args) {return CSProActionInvoker.$Impl.run(this,11276,args);}
executeAsync(args) {return CSProActionInvoker.$Impl.runAsync(this,11276,args);}
registerAccessToken(args) {return CSProActionInvoker.$Impl.run(this,13052,args);}
registerAccessTokenAsync(args) {return CSProActionInvoker.$Impl.runAsync(this,13052,args);}

Application = {
  getFormFile: (args)=>{return CSProActionInvoker.$Impl.run(this,49910,args);},
  getFormFileAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,49910,args);},
  getQuestionnaireContent: (args)=>{return CSProActionInvoker.$Impl.run(this,50614,args);},
  getQuestionnaireContentAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,50614,args);},
  getQuestionText: (args)=>{return CSProActionInvoker.$Impl.run(this,60242,args);},
  getQuestionTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,60242,args);}
};

Clipboard = {
  getText: (args)=>{return CSProActionInvoker.$Impl.run(this,47271,args);},
  getTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,47271,args);},
  putText: (args)=>{return CSProActionInvoker.$Impl.run(this,33682,args);},
  putTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,33682,args);}
};

Data = {
  getCase: (args)=>{return CSProActionInvoker.$Impl.run(this,36632,args);},
  getCaseAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,36632,args);}
};

Dictionary = {
  getDictionary: (args)=>{return CSProActionInvoker.$Impl.run(this,43928,args);},
  getDictionaryAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,43928,args);}
};

File = {
  copy: (args)=>{return CSProActionInvoker.$Impl.run(this,20829,args);},
  copyAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,20829,args);},
  readBytes: (args)=>{return CSProActionInvoker.$Impl.run(this,16986,args);},
  readBytesAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,16986,args);},
  readLines: (args)=>{return CSProActionInvoker.$Impl.run(this,50886,args);},
  readLinesAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,50886,args);},
  readText: (args)=>{return CSProActionInvoker.$Impl.run(this,38897,args);},
  readTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,38897,args);},
  writeBytes: (args)=>{return CSProActionInvoker.$Impl.run(this,33892,args);},
  writeBytesAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,33892,args);},
  writeLines: (args)=>{return CSProActionInvoker.$Impl.run(this,22252,args);},
  writeLinesAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,22252,args);},
  writeText: (args)=>{return CSProActionInvoker.$Impl.run(this,16500,args);},
  writeTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,16500,args);}
};

Hash = {
  createHash: (args)=>{return CSProActionInvoker.$Impl.run(this,30568,args);},
  createHashAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,30568,args);},
  createMd5: (args)=>{return CSProActionInvoker.$Impl.run(this,9799,args);},
  createMd5Async: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,9799,args);}
};

Localhost = {
  mapActionResult: (args)=>{return CSProActionInvoker.$Impl.run(this,47687,args);},
  mapActionResultAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,47687,args);},
  mapFile: (args)=>{return CSProActionInvoker.$Impl.run(this,22391,args);},
  mapFileAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,22391,args);},
  mapSymbol: (args)=>{return CSProActionInvoker.$Impl.run(this,21848,args);},
  mapSymbolAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,21848,args);},
  mapText: (args)=>{return CSProActionInvoker.$Impl.run(this,62691,args);},
  mapTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,62691,args);}
};

Logic = {
  eval: (args)=>{return CSProActionInvoker.$Impl.run(this,50799,args);},
  evalAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,50799,args);},
  getSymbol: (args)=>{return CSProActionInvoker.$Impl.run(this,44034,args);},
  getSymbolAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,44034,args);},
  getSymbolMetadata: (args)=>{return CSProActionInvoker.$Impl.run(this,4818,args);},
  getSymbolMetadataAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,4818,args);},
  getSymbolValue: (args)=>{return CSProActionInvoker.$Impl.run(this,22923,args);},
  getSymbolValueAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,22923,args);},
  invoke: (args)=>{return CSProActionInvoker.$Impl.run(this,41927,args);},
  invokeAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,41927,args);},
  updateSymbolValue: (args)=>{return CSProActionInvoker.$Impl.run(this,17970,args);},
  updateSymbolValueAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,17970,args);}
};

Message = {
  formatText: (args)=>{return CSProActionInvoker.$Impl.run(this,31960,args);},
  formatTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,31960,args);},
  getText: (args)=>{return CSProActionInvoker.$Impl.run(this,449,args);},
  getTextAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,449,args);}
};

Path = {
  createDirectory: (args)=>{return CSProActionInvoker.$Impl.run(this,9881,args);},
  createDirectoryAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,9881,args);},
  getDirectoryListing: (args)=>{return CSProActionInvoker.$Impl.run(this,36724,args);},
  getDirectoryListingAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,36724,args);},
  getPathInfo: (args)=>{return CSProActionInvoker.$Impl.run(this,52302,args);},
  getPathInfoAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,52302,args);},
  getSpecialPaths: (args)=>{return CSProActionInvoker.$Impl.run(this,41709,args);},
  getSpecialPathsAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,41709,args);},
  selectFile: (args)=>{return CSProActionInvoker.$Impl.run(this,28744,args);},
  selectFileAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,28744,args);},
  showFileDialog: (args)=>{return CSProActionInvoker.$Impl.run(this,23553,args);},
  showFileDialogAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,23553,args);}
};

Settings = {
  getValue: (args)=>{return CSProActionInvoker.$Impl.run(this,58779,args);},
  getValueAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,58779,args);},
  putValue: (args)=>{return CSProActionInvoker.$Impl.run(this,28521,args);},
  putValueAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,28521,args);}
};

Sqlite = {
  close: (args)=>{return CSProActionInvoker.$Impl.run(this,14130,args);},
  closeAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,14130,args);},
  exec: (args)=>{return CSProActionInvoker.$Impl.run(this,63594,args);},
  execAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,63594,args);},
  open: (args)=>{return CSProActionInvoker.$Impl.run(this,58055,args);},
  openAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,58055,args);},
  rekey: (args)=>{return CSProActionInvoker.$Impl.run(this,3856,args);},
  rekeyAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,3856,args);}
};

System = {
  getSharableUri: (args)=>{return CSProActionInvoker.$Impl.run(this,20827,args);},
  getSharableUriAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,20827,args);},
  selectDocument: (args)=>{return CSProActionInvoker.$Impl.run(this,30644,args);},
  selectDocumentAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,30644,args);}
};

UI = {
  alert: (args)=>{return CSProActionInvoker.$Impl.run(this,31133,args);},
  alertAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,31133,args);},
  closeDialog: (args)=>{return CSProActionInvoker.$Impl.run(this,60265,args);},
  closeDialogAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,60265,args);},
  enumerateWebViews: (args)=>{return CSProActionInvoker.$Impl.run(this,30914,args);},
  enumerateWebViewsAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,30914,args);},
  getDisplayOptions: (args)=>{return CSProActionInvoker.$Impl.run(this,57563,args);},
  getDisplayOptionsAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,57563,args);},
  getInputData: (args)=>{return CSProActionInvoker.$Impl.run(this,57200,args);},
  getInputDataAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,57200,args);},
  getMaxDisplayDimensions: (args)=>{return CSProActionInvoker.$Impl.run(this,30339,args);},
  getMaxDisplayDimensionsAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,30339,args);},
  postWebMessage: (args)=>{return CSProActionInvoker.$Impl.run(this,31457,args);},
  postWebMessageAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,31457,args);},
  setDisplayOptions: (args)=>{return CSProActionInvoker.$Impl.run(this,62732,args);},
  setDisplayOptionsAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,62732,args);},
  showDialog: (args)=>{return CSProActionInvoker.$Impl.run(this,49835,args);},
  showDialogAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,49835,args);},
  view: (args)=>{return CSProActionInvoker.$Impl.run(this,50017,args);},
  viewAsync: (args)=>{return CSProActionInvoker.$Impl.runAsync(this,50017,args);}
};

}
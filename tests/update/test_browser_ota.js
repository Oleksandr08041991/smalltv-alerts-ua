// Exercises the actual embedded OTA browser script against mock GitHub/CDN
// and the device's /update endpoint. No ESP8266 TLS or live network needed.
const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const html=fs.readFileSync('src/webui.html','utf8');
const start=html.indexOf('// Browser-assisted GitHub OTA for the ESP8266');
const end=html.indexOf('// settings backup',start);
assert(start>=0 && end>start,'browser OTA code missing');
const script=html.slice(start,end);
const elements={
 ghMsg:{textContent:''},ghUpBtn:{disabled:true},
 chkBtn:{disabled:false},fwVer:{textContent:'SmallTV 3.7.2'}
};
const firmware=new Uint8Array(778160);firmware[0]=0xE9;
const calls=[],timers=[];
const release={tag_name:'v3.7.3',assets:[{name:'smalltv-mod-firmware.bin',size:firmware.length,digest:null}]};
let uploaded=null,confirmed=0;
class Xhr{
 constructor(){this.upload={}}
 open(method,url){this.method=method;this.url=url}
 send(body){
  uploaded={method:this.method,url:this.url,body};
  this.status=200;this.responseText='OK';this.onload();
 }
}
class Form{
 constructor(){this.parts={}}
 append(name,data,filename){this.parts[name]={data,filename}}
}
const ctx={
 $:name=>elements[name],C:{version:'3.7.2'},console,
 window:{crypto:{}},crypto:{},XMLHttpRequest:Xhr,FormData:Form,Blob,
 Math,Uint8Array,String,Array,Error,encodeURIComponent,
 location:{reload:()=>{}},setTimeout:(fn,ms)=>timers.push([fn,ms]),
 confirm:()=>{confirmed++;return true},
 fetch:async function(url){
  calls.push(url);
  if(url.includes('api.github.com'))return {ok:true,json:async()=>release};
  if(url.includes('cdn.jsdelivr.net'))return {ok:true,arrayBuffer:async()=>firmware.buffer};
  throw new Error('Unexpected URL '+url);
 }
};
vm.createContext(ctx);
vm.runInContext(script,ctx);
async function test(){
 await vm.runInContext('checkUpdate()',ctx);
 assert.equal(elements.ghUpBtn.disabled,false,'Update button must unlock for newer release');
 assert(elements.ghMsg.textContent.includes('v3.7.3'));
 await vm.runInContext('selfUpdate()',ctx);
 assert.equal(confirmed,1);
 assert(uploaded && uploaded.method==='POST' && uploaded.url==='/update');
 assert.equal(uploaded.body.parts.firmware.filename,'smalltv-mod-firmware.bin');
 assert.equal(uploaded.body.parts.firmware.data.size,firmware.length);
 assert.equal(timers.length,1,'Successful flash should schedule UI refresh');
 assert(elements.ghMsg.textContent.includes('перезавантажується'));
 assert(calls.some(x=>x.includes('api.github.com')),'Release checked from browser');
 assert(calls.some(x=>x.includes('cdn.jsdelivr.net')),'Firmware downloaded from CORS CDN');

 // False-positive up-to-date: no upload should be available.
 elements.fwVer.textContent='SmallTV 3.7.3';
 await vm.runInContext('checkUpdate()',ctx);
 assert.equal(elements.ghUpBtn.disabled,true);
 assert(elements.ghMsg.textContent.includes('актуальна'));
 // Invalid binary is blocked before posting to the device.
 elements.fwVer.textContent='SmallTV 3.7.2';
 await vm.runInContext('checkUpdate()',ctx);
 ctx.fetch=async(url)=>url.includes('api.github.com')
     ?{ok:true,json:async()=>release}
     :{ok:true,arrayBuffer:async()=>new Uint8Array(firmware.length).buffer};
 const before=uploaded;
 await vm.runInContext('selfUpdate()',ctx);
 assert.strictEqual(uploaded,before,'Invalid binary must not reach /update');
 assert(elements.ghMsg.textContent.includes('Помилка OTA'));
 console.log('Browser-assisted OTA checks passed: CORS CDN, versioning, binary validation, /update upload and failure handling.');
}
test().catch(e=>{console.error(e);process.exit(1)});

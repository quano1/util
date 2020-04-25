#!/usr/bin/env node

Object.defineProperty(global, '__stack', {
get: function() {
        var orig = Error.prepareStackTrace;
        Error.prepareStackTrace = function(_, stack) {
            return stack;
        };
        var err = new Error;
        Error.captureStackTrace(err, arguments.callee);
        var stack = err.stack;
        Error.prepareStackTrace = orig;
        return stack;
    }
});

Object.defineProperty(global, '__line', {
get: function() {
        return __stack[1].getLineNumber();
    }
});

Object.defineProperty(global, '__function', {
get: function() {
        return __stack[1].getFunctionName();
    }
});

Object.defineProperty(global, '__file', {
get: function() {
        return __stack[1].getFileName();
    }
});

var dgram = require('dgram');
var svrBC = dgram.createSocket('udp4');

svrBC.on('listening', () => {
    var address = svrBC.address();
    console.log(`${__file} [${__line}] UDP is listening on ${address.address} : ${address.port}`);
});

svrBC.on('message', (buff, remote) => {

    // console.log(` ${__file} [${__line}] ${remote.address} : ${remote.port} - ${buff}`);
    // var date = new Date();
    // var myip = require('quick-local-ip');
    // var IPserver = myip.getLocalIP4();
    // try {
    //     var obj = JSON.parse(buff);
    //     console.log(obj);
    // }
    // catch(err) {
    //     // console.error(err)
    // }

    // if(!obj)
    if(buff.length > 0)
    {
        console.log(`'${buff.toString()}'`);
    }

    // buff = Buffer.allocUnsafe(1);
    // buff.writeInt8(0,0);
    // buff = Buffer.from(`{"ip_server":"${IPserver}"}`);

    // svrBC.send(buff, 0, buff.length, remote.port, remote.address, (err, bytes) => {
    //     if (err) throw err;
    // });

});

// svrBC.bind(6969, "localhost");

var buff = Buffer.from("C");

svrBC.send(buff, 0, buff.length, 65501, "localhost", (err, bytes) => {
    if (err) throw err;
});


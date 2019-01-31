
// var appElement = document.querySelector('[ng-app=AudioStreaming]');
// var appScope = angular.element(appElement).scope();
// var controllerScope = appScope.$$childHead;


function wrap_inc(v, w, i) {
    if((v+i) >= w) return 0;
    return v+i;
}

function wrap_dec(v, w, i) {
    if((v-i) < 0) return w;
    return v-i;
}

var MyScene = cc.Scene.extend({

    _layer: null,
    _wfdata: null,
    _dirty: false,
    _dirtyAccel: false,
    _accelData: null,

    // _wfSample: 44100,
    // _wfFrame: 512,
    // _wfPeriod: 5,
    _wfHeight: 200,
    _wIdx: 1024,

    ctor: function () {
        this._super();
        this.init();
    },

    init: function ()
    {
        const winSize = cc.director.getWinSize();
        this._wfdata = new Float32Array(winSize.width);
        this._wIdx = winSize.width;
        this._layer = new cc.LayerColor(cc.color(0x21,0x21,0x21,0xff), winSize.width, winSize.height);
        this.addChild(this._layer, 0, 100);
    },

    onEnter: function () {
        this._super();
        // const size = cc.director.getWinSize();

        var audLine = new cc.DrawNode();
        this._layer.addChild(audLine, 10, 100);

        const winSize = cc.director.getWinSize();
        var centerPos = cc.p(winSize.width / 2, winSize.height / 2);
        //drawSegment
        audLine.drawSegment(cc.p(0, winSize.height/2), cc.p(winSize.width, winSize.height/2), 3, cc.color(0x8b, 0xc3, 0x4a, 255));

        var lbGx = cc.LabelTTF.create("Gx: ", "Arial", 24);
        lbGx.setPosition(cc.p(10, winSize.height - 20));
        lbGx.setAnchorPoint(0,0.5);
        this._layer.addChild(lbGx, 100, 101);

        var lbGy = cc.LabelTTF.create("Gy: ", "Arial", 24);
        lbGy.setPosition(cc.p(10, winSize.height - 60));
        lbGy.setAnchorPoint(0,0.5);
        this._layer.addChild(lbGy, 100, 102);

        var lbGz = cc.LabelTTF.create("Gz: ", "Arial", 24);
        lbGz.setPosition(cc.p(10, winSize.height - 100));
        lbGz.setAnchorPoint(0,0.5);
        this._layer.addChild(lbGz, 100, 103);


        var lbAx = cc.LabelTTF.create("Ax: ", "Arial", 24);
        lbAx.setPosition(cc.p(10, 100));
        lbAx.setAnchorPoint(0,0.5);
        this._layer.addChild(lbAx, 100, 104);

        var lbAy = cc.LabelTTF.create("Ay: ", "Arial", 24);
        lbAy.setPosition(cc.p(10, 60));
        lbAy.setAnchorPoint(0,0.5);
        this._layer.addChild(lbAy, 100, 105);

        var lbAz = cc.LabelTTF.create("Az: ", "Arial", 24);
        lbAz.setPosition(cc.p(10, 20));
        lbAz.setAnchorPoint(0,0.5);
        this._layer.addChild(lbAz, 100, 106);

        // this._dirty = true;

        // // 2196F3
        // var rotX = new cc.DrawNode();
        // rotX.drawSegment(cc.p(-50, 0),cc.p(50, 0), 3, cc.color(0xf4, 0x43, 0x36, 255));
        // rotX.setPosition(cc.p(100, winSize.height - 100));
        // this._layer.addChild(rotX, 11, 101);

        // var rotY = new cc.DrawNode();
        // rotY.drawSegment(cc.p(-35, -50),cc.p(-35, 50), 3, cc.color(0x8b, 0xc3, 0x4a, 255));
        // rotY.drawSegment(cc.p(-35, 50),cc.p(35, 50), 3, cc.color(0x8b, 0xc3, 0x4a, 255));
        // rotY.drawSegment(cc.p(35, 50),cc.p(35, -50), 3, cc.color(0x8b, 0xc3, 0x4a, 255));
        // rotY.drawSegment(cc.p(35, -50),cc.p(-35, -50), 3, cc.color(0x8b, 0xc3, 0x4a, 255));
        // rotY.setPosition(cc.p(250, winSize.height - 100));
        // this._layer.addChild(rotY, 11, 102);

        // // F44336
        // var rotZ = new cc.DrawNode();
        // rotZ.drawSegment(cc.p(-50, 0),cc.p(50, 0), 3, cc.color(0x21, 0x96, 0xf3, 255));
        // rotZ.setPosition(cc.p(400, winSize.height - 100));
        // this._layer.addChild(rotZ, 11, 103);
        
        this.scheduleUpdate();
    },

    update: function(dt) {
        this._super();
        // if (this._wfdata) console.log(this._wfdata.length);
        // console.log(this._dirty);
        if(this._dirty)
        {
            var scene = cc.director.getRunningScene();
            var audLine = this._layer.getChildByTag(100);
            audLine.clear();
            var vertices = [];
            const winSize = cc.director.getWinSize();
            const centerPos = cc.p(winSize.width / 2, winSize.height / 2);
            // for (var i = 0; i < this._wfdata.length; i++)

            vertices.push(cc.p(0, centerPos.y + this._wfdata[this._wIdx] * this._wfHeight));
            this._wIdx = wrap_inc(this._wIdx, this._wfdata.length, 1);

            for (var i=this._wIdx, index=1;
                index < this._wfdata.length;
                i=wrap_inc(i, this._wfdata.length, 1), index++)
            {
                vertices.push(cc.p(index, centerPos.y + this._wfdata[i] * this._wfHeight));
            };

            // console.log(`i:${i} wid:${this._wIdx} index:${index}`);

            for (var i = 0; i < vertices.length - 1; i++) {
                audLine.drawSegment(vertices[i], vertices[i+1], 3, cc.color(0x8b, 0xc3, 0x4a, 255));
            };

            // console.log('update: ' + dt);
            this._dirty = false;
        }

        if(this._dirtyAccel)
        {
            this._layer.getChildByTag(101).setString("Gx: " + this._accelData[0]);
            this._layer.getChildByTag(102).setString("Gy: " + this._accelData[1]);
            this._layer.getChildByTag(103).setString("Gz: " + this._accelData[2]);
            this._layer.getChildByTag(104).setString("Ax: " + this._accelData[3]);
            this._layer.getChildByTag(105).setString("Ay: " + this._accelData[4]);
            this._layer.getChildByTag(106).setString("Az: " + this._accelData[5]);

            // this._layer.getChildByTag(101).setString("Gx: " + cc.radiansToDegrees(this._accelData[0]));
            // this._layer.getChildByTag(102).setString("Gy: " + cc.radiansToDegrees(this._accelData[1]));
            // this._layer.getChildByTag(103).setString("Gz: " + cc.radiansToDegrees(this._accelData[2]));
            // this._layer.getChildByTag(104).setString("Ax: " + cc.radiansToDegrees(this._accelData[3]));
            // this._layer.getChildByTag(105).setString("Ay: " + cc.radiansToDegrees(this._accelData[4]));
            // this._layer.getChildByTag(106).setString("Az: " + cc.radiansToDegrees(this._accelData[5]));
            this._dirtyAccel = false;
        }

        // var rotX = this._layer.getChildByTag(101);
        // rotX.setRotation(rotX.getRotation() - 1);

        // var rotY = this._layer.getChildByTag(102);
        // rotY.setRotation(rotY.getRotation() + 1);

        // var rotZ = this._layer.getChildByTag(103);
        // rotZ.setRotation(rotZ.getRotation() + 1);
    },

    updateWf: function(val) {
        this._wfdata[this._wIdx] = val;
        this._wIdx = wrap_dec(this._wIdx, this._wfdata.length, 1);
        this._dirty = true;
    },

    updateAC: function(data) {
        this._accelData = data;
        this._dirtyAccel = true;
    },
});

// MyScene.prototype.updateWf = function(val) {
//     this._wfdata[this._wIdx] = val;
//     this._wIdx = wrap_dec(this._wIdx, this._wfdata.length, 1);
//     this._dirty = true;
// }

// MyScene.prototype.updateAC = function(val) {
//     this._accelData = data;
//     this._dirtyAccel = true;
// }

// var scene = null;
// cc.game.onStart = function(){
//     if(!cc.sys.isNative && document.getElementById("cocosLoading")) //If referenced loading.js, please remove it
//       document.body.removeChild(document.getElementById("cocosLoading"));
//     // winsize = cc.director.getWinSize();

//     cc.view.setDesignResolutionSize(1024, 800, cc.ResolutionPolicy.SHOW_ALL);
//     cc.view.resizeWithBrowserSize(true);
//     //load resources
//     cc.LoaderScene.preload([], function () {
//         scene = new MyScene();
//         cc.director.pushScene(scene);
//         // console.log(scene);
//         // scene = cc.director.getRunningScene();
//         // console.log(scene);
//     }, this);
// };

// cc.game.run("gameCanvas");
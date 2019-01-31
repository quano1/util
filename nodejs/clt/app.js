
angular.module('lltlog', ['ngMaterial', 'ngRoute'])
.controller('AppCtrl', ['$scope', '$route', function($scope, $route) {
  $scope.imagePath = 'img/washedout.png';
  this.$route = $route;
  
  	var scene = null;

    // $scope.init = function() {
    // console.log("init");
    // }

  // $scope.$on('$viewContentLoaded', function() {
      //call it here
    console.log("on viewContentLoaded");
          cc.game.onStart = function(){
        if(!cc.sys.isNative && document.getElementById("cocosLoading")) //If referenced loading.js, please remove it
          document.body.removeChild(document.getElementById("cocosLoading"));
        winsize = cc.director.getWinSize();
        console.log(winsize);

        cc.view.setDesignResolutionSize(1024, 800, cc.ResolutionPolicy.SHOW_ALL);
        cc.view.resizeWithBrowserSize(true);
        //load resources
        cc.LoaderScene.preload([], function () {
            scene = new MyScene();
            cc.director.pushScene(scene);
            console.log("preload");

        }, this);
    };
    console.log("run gameCanvas");
    cc.game.run("gameCanvas");
  // });

 //  $scope.$watch('$viewContentLoaded', function(){
 //    // do something
 //    console.log("watch viewContentLoaded");
 // });

}])
.config(function($mdThemingProvider) {
  $mdThemingProvider.theme('dark-grey').backgroundPalette('grey').dark();
  $mdThemingProvider.theme('dark-orange').backgroundPalette('orange').dark();
  $mdThemingProvider.theme('dark-purple').backgroundPalette('deep-purple').dark();
  $mdThemingProvider.theme('dark-blue').backgroundPalette('blue').dark();
});

window.onload = function(){
//     var scene = null;
    // cc.game.onStart = function(){
    //     if(!cc.sys.isNative && document.getElementById("cocosLoading")) //If referenced loading.js, please remove it
    //       document.body.removeChild(document.getElementById("cocosLoading"));
    //     winsize = cc.director.getWinSize();
    //     console.log(winsize);

    //     cc.view.setDesignResolutionSize(1024, 800, cc.ResolutionPolicy.SHOW_ALL);
    //     cc.view.resizeWithBrowserSize(true);
    //     //load resources
    //     cc.LoaderScene.preload([], function () {
    //         scene = new MyScene();
    //         cc.director.pushScene(scene);
    //         console.log("preload");
    //         console.log(scene);

    //     }, this);
    // };

    // console.log("run gameCanvas");
    // cc.game.run("gameCanvas");
};

/**
Copyright 2018 Google LLC. All Rights Reserved. 
Use of this source code is governed by an MIT-style license that can be found
in the LICENSE file at http://material.angularjs.org/HEAD/license.
**/
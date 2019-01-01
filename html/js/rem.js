new function (){
       var _self = this;
       _self.width = 750;//设置默认最大宽度
       _self.fontSize = 100;//默认字体大小
       _self.widthProportion = function(){var p = (document.body&&document.body.clientWidth||document.getElementsByTagName("html")[0].offsetWidth)/_self.width;return p>1?1:p<0.32?0.32:p;};
       _self.changePage = function(){
           document.getElementsByTagName("html")[0].setAttribute("style","font-size:"+_self.widthProportion()*_self.fontSize+"px !important");
       }
       _self.changePage(); 
       window.addEventListener('resize',function(){_self.changePage();},false);
    };
    
function tables(ev, sl) {
	var index = $(ev).index();
	$(sl).eq(index).show().siblings().hide();
}

function active(ev) {
	$(ev).addClass('active').siblings().removeClass('active');
}

function shows(ev){
	ev.show();
}

function hides(ev){
	ev.hide();
}


jQuery.mGetRandom = function (x, y) {
	return parseInt(Math.random() * (y - x + 1) + x);
};


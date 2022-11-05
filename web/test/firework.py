firework=[[300,200,-2.5,-2.5,"red"],[300,210,2.5,-5,"green"],[305,205,2.5,-7.5,"orange"],[295,203,1.5,-7,"lightblue"],[303,198,-3,-4,"blue"],[303,204,2.4,-8.8,"#FA0010"],[301,209,-2.2,-9.3,"#10FA40"],[302,206,-2.5,-9.5,"#FA9A10"],[297,202,-1.0,-8,"#1010AA"],[296,201,3.2,-10.6,"#10FA89"]]
sceneClear()
for num in range(23):
	centerx=random()*200-400
	centery=random()*50-100
	a=firework
	for no in range(10):
		a[no][2]=(random()*20)-10
		a[no][3]=-(random()*20)-5
	for iteration in range(100):
		delay=getTick()
		sceneSwap()     
		sceneClear()    
		for no in range(10):
			setColor(a[no][4])
			x = a[no][0]
			y = a[no][1]
			acc = a[no][3]
			drawCircle(x+centerx-10,y+centery-10,x+10+centerx,y+10+centery)
			a[no][0] = x + a[no][2]
			a[no][1] = y + acc
			a[no][3] = acc + 0.3
		sceneSwap()     
		while (getTick()-delay<10):
			delay=delay
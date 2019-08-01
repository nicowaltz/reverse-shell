def progressbar(ratio, action="Loading"):
		bar=""
		if ratio > 1: ratio = 1

		amount = int(30*ratio)
		percentage = int(100*ratio)
		for i in range(amount):
			bar += "█"

		if 30*ratio > amount + 0.5:
			bar += "▌"
			amount += 1
			

		for i in range(30 - amount):
			bar += " "

		print("\r%s: %s| %d%% " % (action, bar, percentage), end='')



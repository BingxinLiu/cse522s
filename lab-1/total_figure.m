x = [6 7 8 9];
y1 = [-0.262271	-0.246179	-0.194163	-0.124896];
y1err = [0.270031	0.185763	0.163712	0.044447];
y2 = [-0.283762	-0.272089	-0.235648	-0.202710];
y2err = [0.074982	0.052032	0.088261	0.052205];
figure
plot(x, y2, x, y1);
errorbar(x, y2, y2err);
hold on;
errorbar(x, y1, y1err);
title("Total Time v.s Matrix Size");
xlabel("Matrix Size(log)");
ylabel("Total Time(log)");
legend("demand\_paging", "pre-paging");
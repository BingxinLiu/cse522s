x = [6 7 8 9];
y1 = [-0.263221	-0.246759	-0.194382	-0.125041];
y1err = [0.273446	0.187384	0.164231	0.044543];
y2 = [-0.286463	-0.273238	-0.236276	-0.203052];
y2err = [0.077029	0.052764	0.088480	0.052285];
figure
plot(x, y1, x, y2);
errorbar(x, y1, y1err);
hold on;
errorbar(x, y2, y2err);
title("Calculating Time v.s Matrix Size");
xlabel("Matrix Size(log)");
ylabel("Calculating Time(log)");
legend("demand\_paging", "pre-paging");
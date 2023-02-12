# 规则说明

本游戏为二人游戏，游戏双方分别被称作甲方和乙方。

棋盘是圆形的，长得像钟表的，有从顺时针方向编号1到12共12个空位。

初始状态下，有编号为1到12的棋子随机地分配并摆放到棋盘上的12个空位上。

以下把编号为n的棋子被正好摆放到编号为n的空位的这种情况，称之为编号为n的棋子处于“正位”。

初始状态下不允许任何棋子处于“正位”。如果存在，则需要重新随机分配直到没有棋子处于“正位”为止。

行棋顺序为，先由乙方先行棋，之后双方轮流行棋。行棋分3种类型：(1) 走子；(2) 摆子 (3)  不走，即放弃本次行棋机会，转由对方走棋。

"走子" 是指，一方拿起棋盘上的任意一枚棋子，此棋子的编号是n，则顺时针走n步，若走n步后停留的空位有其它棋子，则要把它吃掉。任何一方吃掉的棋子，如果这个棋子的编号是奇数，就交到甲方的手上，如果是偶数就交到乙方的手上。这些拿在手上的棋子将用于落子。

注意：对方上一步刚走过的棋子，己方在这一步不能再重复拿来走。

处于“正位”的棋子，既不能再移动，也不能被吃掉。因此，假设编号为n的棋子顺时针走n步后，落在的空位上有处于正位的棋子，则这种情况下，这个编号为n棋子是不能被移动的。

特别值得一提的是，对于编号为12的棋子，它走一圈之后正好吃到自己，因此我们会见到乙方直接把编号为12的棋子取出棋盘，拿到手上。当然理论上甲方也可以把编号为12的棋子从棋盘上取出之后交到乙方手上。

“落子” 是指，一方把手中的己方棋子落在它的正位上，比如将编号为n的棋子放在编号为n的空位上。如果这个空位上有其它棋子，也要把它吃掉。

落子时吃掉的棋子如果是奇数，就交到甲方的手上，如果是偶数，就交到乙方的手上。

如果落子时吃掉了对方的棋子，则己方多一次行棋的机会，即下一步对方不能行棋，还是轮到自己行棋。

何一方都可以主动放弃本回合的行棋，轮到对方行棋。

这个游戏有个奇特的地方是，在棋盘上不处于"正位"的棋子，甲乙双方都可以移动。也就是说甲方可以移动偶数棋子，乙方也可移奇数棋子。

游戏有四种结局：“赢棋”、“输棋”、“双赢”、“双输”。关于胜负的评判如下：

第一个把己方棋子全部放到“正位”者“赢棋”，如果对方接下来一步也达成了将己方棋子都放到“正位”上，则为“双赢”。

如果双方都不可能把棋子落到“正位”，则“双输”。

若甲乙双方接连放弃行棋，则判双方都输棋，即也是“双输”。

如果某一方的最后一步是落子吃子，则是此方“赢棋“，而不是”双赢“。因为落子时吃对方棋子，是多一次行棋机会的。

若双方追求公平，可用已证实为”双赢“的随机局面来开局。

# 游戏策略

这个游戏的每一局棋的情况都是完全不同的，这考验着棋手的思维灵敏度。

作为一种**随机开局**的**不等目**弈棋，混乱时钟并**不保证**每一个开局对于双方而言是绝对公平的。

初始局面的好坏大概用以下方式判断：

* 一方的所有棋子是否都至少有一个途径能归放至正位。

* 对于乙方而言，开局时，它在单数位的棋子数量越多，应该就越处于劣势。

* 把己方棋子取出并移至正位需要多少步，考虑到对方的行动和干扰。

* 甲方的「1」跟「11」也是两个比较难处理的棋子。

当然局面是非常复杂的，通常我们需要在几个方案之间找到最佳方案，在保证己方能归至正位的同时，最大限度的推迟对方，给对方造成困难，甚至考虑把对方卡死在路上。

每一个棋子都因为行走距离不同而有不同的技能，本身是可以移动的棋子，也可以被别的棋子吃掉，这正是本棋的有趣之处。

「双赢」和「皆输」是本棋的特点之一，其它的棋应该就只有「和棋」一说。那么为甚么会这样？因为，如果只能在「双赢」和「皆输」之间作出选择，那么大家肯定都会选择「双赢」。

本棋如果有广告词，或许可说「要赢一起赢，要输一起输」。刚测试完的一盘棋就是这样：乙方把「11」移到了铁3前，结果甲方必须走「5」，路上会吃到「4」，否则「4」也无法取到。如果甲方不按这个路线走，那么他也就别想赢了！这个「5」走了五步才吃到「11」，**一路上不是乙方给他开绿灯，他根本赢不了。**

除此以外，本棋还有个奇特的地方是，在场上不处于正位的棋子，谁都可以移动。也就是说甲方可以移双数棋子，乙方也可移单数棋。因此有时你会见到双方在帮对方走子或吃子。

# 局面评估

## 活动性

初始每个棋子都有「活动性」，就是它可能走到的地方，写进一个数组。

如：

```python
move_available = {
    1:[1,2,...],
    3:[2,5,8,11]
}
```

当一个棋子被吃掉，则它失去活动性（从机动类中移除）。

当一方行动结束，若棋子处于正位，则所有棋子的活动性要重新计算。

## 卡住判定

当一个棋子无法往前走一步，它的机动性是空。换句话说，这个棋子被卡住了。

这些被卡住的棋子所处的位置，如果处于其它棋子的机动范围，则它能被走子解救。否则不能。

如果它处于一个己方能落子的地方，己方手上也有这颗棋子，则能被落子解救。

如果它不处于己方能落子的地方，或者两颗原本能互相落子解救的棋都同时被卡住（双扣），则不能被落子解救。

一方只要有一颗无法走且不能被解救的棋子，那他就是「被卡住」了。

## 胜负提前判定 

甲方被卡住，己方全部到手：甲输乙赢

乙方被卡住，甲方全部到手：乙输甲赢

双方都被卡住，同输。

双方都没有被卡且不会被卡，场上正位棋子数量判断输赢，相差1为共赢。

## 行动优先

甲方永远不会主动走12

一方似乎不会走使自己被卡住的棋，但是其实未必，因为卡住后，他就可以停在那里等待有机动性的棋子前来解救。

# 编译方法

## Linux

执行 `make all`。

## Windows

使用 Visual C++ 打开 sln 文件编译。

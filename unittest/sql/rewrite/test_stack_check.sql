#before test set stack limit to small value
select * from (select c1 from t1) v1 join (
  select 1 as b union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
  union all select 2 as b union all select 2 as b
) v2 on v1.c1 = v2.b;

###case2
select money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money+money
from tm where money > 0;

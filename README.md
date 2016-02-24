MPHF
====

Minimal perfect hash function (BDZ algorithm). Modern C++ implementation.

need to store g array and h0, h1, h3 functions
every element in g is between [0, 2], so theoretically we need just two extra bits. In this implementation we really need 1.3`*(sizeof(Key) + sizeof(Value) + 1).

```
for a key, hash value = 
{
    i = (g[h0(key)] + g[h1(key)] + g[h2(key)]) % 3
    return hi(key)
}
```

# C Container Collection (CCC)

The C Container Collection offers a variety of containers for C programmers who want fine-grained control of memory in their programs. All containers offer both allocating and non-allocating interfaces. For the motivations of why such a library is helpful in C read on.

## Installation

Currently, this library supports a manual installation via CMake. See the [INSTALL.md](INSTALL.md) file for more details.

## Quick Start

- Read the [DOCS](https://agl-alexglopez.github.io/ccc).
- Read [types.h](https://agl-alexglopez.github.io/ccc/types_8h.html) to understand the `ccc_alloc_fn` interface.
- Read the [header](https://agl-alexglopez.github.io/ccc/files.html) for the desired container to understand its functionality.
- Read about generic [traits.h](https://agl-alexglopez.github.io/ccc/traits_8h.html) shared across containers to make code more succinct.

## Design

### Motivations

Here is the main argument of this library:

**Containers in C should avoid limiting the control and flexibility offered by the C language.**

There are many excellent data structure libraries in C (see the [related](#related) section of this document). However, many of them try to approximate more modern languages like C++ and Rust in both form and function; they implement memory owning containers where the interface implicitly forces you to agree to the container's opinion of how and when memory should be managed. While many accept custom allocators for the container, a core assumption seems to be that it is OK for calls to container functions to have a non-trivial side effect by calling dynamic memory interfaces.

The C Container Collection takes a different approach. When initializing the containers in this library the user chooses if memory management is allowed by the container. If allowed, these containers will manage allocation as one may be used to in higher level languages. However, if allocation is prohibited then these containers offer data structures as their literal interpretation; they structure the data they are given according to the container's invariants with no memory related side effects. The concerns of the container are now separate from those of the programmer. The container takes no part in the scope or lifetime of the programmer's provided memory, simply structuring it within the container as promised by the interface.

The reason for this decision is simple: if people are using C today, in the face of many other modern and safety-minded languages, they should benefit from all the language can provide, even when incorporating a third-party library. This means that they can manage all their memory as they see fit and these containers simply structure it when and how the programmer requires. This design style is not unique. Non-allocating intrusive containers are common in operating system kernel development where the idea of dynamic memory, where memory comes from, and when memory should be allocated or freed becomes more complex due to the structure of all the modules working together behind the scenes to support the user. Embedded developers also stand to benefit from this design. However, I think that all applications can benefit from thinking more carefully about their memory in C.

While not all containers require the user accommodate intrusive elements, when they do it looks like this.

```c
struct key_val
{
    int key;
    int val;
    container_elem elem;
};
```

The handle to the container element is then passed by reference to all functions that require it.

### Allocation

If the non-allocating features are of the most interest to you, this section may not be relevant. However, to support the previously mentioned design motivations, this collection offers the following interface for allocation. The user defines this function and provides it to containers upon initialization.

```c
typedef void *ccc_alloc_fn(void *ptr, size_t size, void *aux);
```

An allocation function implements the following behavior, where ptr is pointer to memory, size is number of bytes to allocate, and aux is a reference to any supplementary information required for allocation, deallocation, or reallocation. The aux parameter is passed to a container upon its initialization and the programmer may choose how to best utilize this reference (read on for more on aux).

- If NULL is provided with a size of 0, NULL is returned.
- If NULL is provided with a non-zero size, new memory is allocated/returned.
- If ptr is non-NULL it has been previously allocated by the alloc function.
- If ptr is non-NULL with non-zero size, ptr is resized to at least size
  size. The pointer returned is NULL if resizing fails. Upon success, the
  pointer returned might not be equal to the pointer provided.
- If ptr is non-NULL and size is 0, ptr is freed and NULL is returned.

One may be tempted to use realloc to check all of these boxes but realloc is implementation defined on some of these points. So, the aux parameter also discourages users from providing realloc. For example, one solution using the standard library allocator might be implemented as follows (aux is not needed):

```c
void *
std_alloc(void *const ptr, size_t const size, void *)
{
    if (!ptr && !size)
    {
        return NULL;
    }
    if (!ptr)
    {
        return malloc(size);
    }
    if (!size)
    {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, size);
}
```

However, the above example is only useful if the standard library allocator is used. Any allocator that implements the required behavior is sufficient. For ideas of how to utilize the aux parameter, see the sample programs. Using custom arena allocators or container compositions are cases when aux is helpful in taming lifetimes and simplifying allocation.

#### Constructors

Another concern for the programmer related to allocation may be constructors and destructors, a C++ shaped peg for a C shaped hole. In general, this library has some limited support for destruction but does not provide an interface for direct constructors as C++ would define them; though this may change.

Consider a constructor. If the container is allowed to allocate, and the user wants to insert a new element, they may see an interface like this (pseudocode as all containers are slightly different).

```c
void *insert(container *c, container_elem *e);
```

Because the user has wrapped the intrusive container element in their type, the entire user type will be written to the new allocation. All interfaces also offer functions that return references to successfully inserted elements if global program state should be set depending on this success. So, if some action beyond setting values needs to be performed, there are multiple opportunities to do so.

#### Destructors

For destructors, the argument is similar but the container does help with this at times. If an action other than freeing the memory of a user type is needed upon removal, there are multiple options in an interface to obtain the element to be removed. Associative containers offer functions that can obtain entries (similar to Rust's Entry API). This reference can then be examined and complex destructor actions can occur before removal. Other containers like lists or priority queues offer references to an element of interest such as front, back, max, min, etc. These can all allow destructor-like actions before removal. The one exception is the following interfaces.

The clear function works for pointer stable containers and flat containers.

```c
result clear(container *c, destructor_fn *fn);
```

The clear and free function works for flat containers.

```c
result clear_and_free(container *c, destructor_fn *fn);
```

The above functions free the resources of the container. Because there is no way to access each element before it is freed when this function is called, a destructor callback can be passed to operate on each element before deallocation.

## Samples

For examples of what code that uses these ideas looks like, read and use the sample programs in the `samples/`. I try to only add non-trivial samples that do something mildly interesting to give a good idea of how to take advantage of this flexible memory philosophy.

## Miscellaneous Why?

- Why callbacks? Freedom for more varied comparisons and allocations. Learn to love auxiliary data. Also debugging your own function is nice.
- Why not header only? I want the library to be readable, maintainable, and updateable, especially if I want to change implementations in the source files. If the user wants to explore the implementation everything should be easily understandable.
- Why not opaque pointers and true implementation hiding? This is not possible in C if the user is in charge of memory. The container types must be complete if the user wishes to store them on the stack or data segment. I try to present a clean interface.
- Why flat maps? Mostly experimenting. Flat maps track the tree structure through indices not pointers. This makes the data structure copyable, relocatable, serializable, or writable to disk at the cost of pointer stability in most cases.
- Why C23? It is a great standard that is pleasant to write with and allows for the expressiveness I like in a library. Clang already covers most of the features used on many platforms. Newer gcc versions also have them covered.

## Related

If these containers do not fit your needs, here are some excellent data structure libraries I have found for C. They are clever, fast, and elegant, taking care of all memory management for you.

- [STC - Smart Template Containers](https://github.com/stclib/STC)
- [C Template Library (CTL)](https://github.com/glouw/ctl)
- [CC: Convenient Containers](https://github.com/JacksonAllan/CC)

template<typename T>
class cyclic_list {
    struct node {
        node *prev = nullptr;
        node *next = nullptr;
        T value;
        node(const T &_value) : value(_value) {}
    };

    node *element = nullptr;

public:
    cyclic_list() = default;
    cyclic_list(const cyclic_list &) = delete;

    cyclic_list(cyclic_list<T> &&other) : element(other.element)
    {
        other.element = nullptr;
    }

    template<typename InputIt>
    cyclic_list(InputIt first, InputIt last)
    {
        for ( ; first != last; ++first)
            push_back(*first);
    }

    void clear()
    {
        if (element != nullptr)
        {
            node *current = element;
            do {
                node *next = current->next;
                delete current;
                current = next;
            } while (current != element);
            element = nullptr;
        }
    }
    ~cyclic_list()
    {
        clear();
    }

    class iterator {
        node *ptr = nullptr;
        bool incremented = false;
        iterator(node *_ptr) : ptr(_ptr) {}
    public:
        iterator() = default;
        iterator &operator++()
        {
            incremented = true;
            ptr = ptr->next;
            return *this;
        }
        iterator &operator--()
        {
            incremented = true;
            ptr = ptr->prev;
            return *this;
        }
        T &operator*() const { return ptr->value; }
        bool operator==(const iterator &rhs) const
        {
            return (ptr == nullptr || incremented || rhs.incremented) && ptr == rhs.ptr;
        }
        bool operator!=(const iterator &rhs) const
        {
            return !(*this == rhs);
        }
        friend cyclic_list;
    };

    T &front() { return element->value; }
    T &back() { return element->prev->value; }
    const T &front() const { return element->value; }
    const T &back() const { return element->prev->value; }

    iterator push_back(const T &value)
    {
        node *const new_node = new node(value);
        if (element == nullptr)
        {
            new_node->prev = new_node->next = new_node;
            element = new_node;
        }
        else
        {
            new_node->prev = element->prev;
            new_node->next = element;
            new_node->prev->next = new_node->next->prev = new_node;
        }
        return new_node;
    }
    iterator push_front(const T &value)
    {
        iterator ret = push_back(value);
        element = ret.ptr;
        return ret;
    }

    iterator begin() const { return element; }
    iterator end()   const { return element; }

    iterator insert(const iterator &pos, const T &value)
    {
        node *ret;
        if (element == nullptr)
        {
            ret = push_back(value).ptr;
        }
        else
        {
            node *const new_node = new node(value);
            node *const next = pos.ptr;
            node *const prev = next->prev;
            prev->next = new_node;
            new_node->prev = prev;
            new_node->next = next;
            next->prev = new_node;
            ret = new_node;
        }
        return ret;
    }

private:
    /* only call this if no cyclic_list points to an element in (_first,_last) */
    void splice(const iterator &_pos, const iterator &_first, const iterator &_last)
    {
        node *const first = _first.ptr;
        node *const last = _last.ptr;
        node *const after_first = first->next;
        node *const before_last = last->prev;
        node *const pos = _pos.ptr;

        node *const first_copy = new node(first->value);
        node *const last_copy = new node(last->value);

        if (pos == nullptr)
        {
            first_copy->prev = last_copy;
            last_copy->next = first_copy;
            element = first_copy;
        }
        else
        {
            node *const before_pos = pos->prev;
            before_pos->next = first_copy;
            first_copy->prev = before_pos;
            last_copy->next = pos;
            pos->prev = last_copy;
        }

        first_copy->next = after_first;
        after_first->prev = first_copy;
        before_last->next = last_copy;
        last_copy->prev = before_last;

        first->next = last;
        last->prev = first;
    }

public:
    void splice(const iterator &_pos, cyclic_list<T> &other, const iterator &_first, const iterator &_last)
    {
        splice(_pos, _first, _last);
        other.element = _first.ptr;
    }

    void reset_head(const iterator &it)
    {
        element = it.ptr;
    }
};

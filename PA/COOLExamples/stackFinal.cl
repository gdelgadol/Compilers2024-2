-- Gabriel Santiago Delgado Lozano
-- Fabio Esteban Murcia Martinez

class Node{
    item: String;
    next: Node;
    prev: Node;

    init(i: String): SELF_TYPE {
        {
            item <- i;
            self;
        }
    };

    setNext(n: Node): SELF_TYPE{
        {
            next <- n;
            self;
        }
    };

    setPrev(n: Node): SELF_TYPE{
        {
            prev <- n;
            self;
        }
    };

    getPrevious(): Node{
        prev
    };

    getNext(): Node{
        next
    };


    printItem(): String{
        item
    };

    printNext(): String{
        next.printItem()
    };

    printPrev(): String{
        prev.printItem()
    };
};

class Stack{
    top: Node;
    size: Int;

    push(i: String): SELF_TYPE{
        let n: Node <- (new Node).init(i)
        in
        {
            size <- size + 1;
            if (isvoid top) then
                top <- n
            else
                {
                    top.setNext(n);
                    n.setPrev(top);
                    top <- n;
                }
            fi;
            self;
        }
    };

    pop(): SELF_TYPE{
        let nil: Node 
        in
        {
            if (isvoid top) then
                self
            else
                {
                top <- top.getPrevious();
                size <- size - 1;
                }
            fi;
            self;
        }
    };

    printTop(): String{
        if (isvoid top) then
            ""
        else
            top.printItem()
        fi
    };

    getsize(): Int{
        size
    };

    tostring(): String{
        let string: String,
            currNode: Node <- top
            in
            {
                while (not(isvoid currNode)) loop
                {
                    string <- string.concat(currNode.printItem());
                    currNode <- currNode.getPrevious();
                }
                pool;
                string;
            }
    };
};

class Main inherits IO{
    main(): Object{
        let s: Stack <- (new Stack)
        in{
            s.push("A");
            s.push("B");
            s.push("C");
            out_string(s.tostring().concat("\n"));

            while ( not (s.getsize() = 0 )) loop
                {
                    out_string(s.tostring().concat("\n"));
                    s.pop();
                }
            pool;
            s;
        }
    };
};

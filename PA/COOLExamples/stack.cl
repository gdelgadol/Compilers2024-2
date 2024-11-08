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
        let string: String <- item in string
    };

    printNext(): String{
        let nextString: String <- next.printItem() in nextString
    };

    printPrev(): String{
        let prevString: String <- prev.printItem() in prevString
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
            let string: String <- top.printItem() in string
        fi
    };

    getsize(): Int{
        let si: Int <- size in si
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

class Main inherits A2I{
    main(): Object{
        let s: Stack <- (new Stack)
            (*
            n: Node <- (new Node).init("Hola "),
            nt: Node <- (new Node).init("Mundo"),
            pt: Node <- (new Node).init("!")
            *)
        in{
            (*
            n.setNext(nt);
            n.setPrev(pt);
            (new IO).out_string(n.printItem().concat("\n"));
            (new IO).out_string(n.printNext().concat("\n"));
            (new IO).out_string(n.printPrev().concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            *)
            s.push("A");
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.push("B");
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.push("C");
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.pop();
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.pop();
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.pop();
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.push("X");
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.push("Y");
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));
            s.push("Z");
            (new IO).out_string(s.printTop().concat("\n"));
            (new IO).out_string(i2a(s.getsize()).concat("\n"));
            (new IO).out_string(s.tostring().concat("\n"));

        }
    };
};
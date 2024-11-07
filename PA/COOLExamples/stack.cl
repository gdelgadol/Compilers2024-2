-- Fabio Murcia
-- Gabriel Santiago Delgado Lozano

class List{
    item: String;
    next: List;
    prev: List;

    init(i: String, n:List): List{
        {
            item <- i;
            prev <- n;
            self;
        }
    };

    flatten(): String {
        let string: String <-
            case item of
                i: Int => i2a(i);
                s: String => s;
                o: Object => {abort(); "";};
            esac
        in
            if(isvoid next) then
                string
            else
                string.concat(next.flatten())
            fi
    };
};

class Stack{
    top: List;
    size: Int;

    push(i: String): {
        if (isvoid item) then
            item <- i;
            size <- size + 1;

    };

    tostring(): String {
        let string: String <-
            case item of
                i: Int => i2a(i);
                s: String => s;
                o: Object => {abort(); "";};
            esac
        in
            if(isvoid next) then
                string
            else
                string.concat(next.flatten())
            fi
    };
};

class Main inherits IO{
    main(): Object{
        let s:Stack <- (new Stack) in
            {
                s.push("A");
                s.push("B");
                s.push("C");
                out_string(s.tostring().concat("\n"));

                while(not ( s.getsize() = 0 )) loop
                    {
                        out_string(s.tostring().concat("\n"));
                        s.pop();
                    }
                pool;
                s;
            }
    };
};
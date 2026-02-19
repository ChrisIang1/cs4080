private Expr comma(){
    Expr expr = equality();

    while(match(COMMA)) {
        Token operator = previous();
        Expr right = equality();
        expr = new Expr.Binary(expr, operator, right);
    }

    return expr;
    }

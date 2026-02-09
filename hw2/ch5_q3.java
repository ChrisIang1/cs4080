package com.craftinginterpreters.lox;

public class ch5_q3 implements Expr.Visitor<String> {

    public String print(Expr expr) {
        return expr.accept(this);
    }

    @Override
    public String visitBinaryExpr(Expr.Binary expr) {
        return expr.left.accept(this) + " " +
                expr.right.accept(this) + " " +
                expr.operator.lexeme;
    }

    @Override
    public String visitGroupingExpr(Expr.Grouping expr) {
        return expr.expression.accept(this);
    }

    @Override
    public String visitLiteralExpr(Expr.Literal expr) {
        if (expr.value == null) return "nil";
        if (expr.value instanceof Double) {
            double d = (Double) expr.value;
            if (d == Math.rint(d)) return Long.toString((long) d);
            return Double.toString(d);
        }
        return expr.value.toString();
    }

    @Override
    public String visitUnaryExpr(Expr.Unary expr) {
        return expr.right.accept(this) + " " + expr.operator.lexeme;
    }

    public static void main(String[] args) {
        Token plusTok  = new Token(TokenType.PLUS,  "+", null, 1);
        Token starTok  = new Token(TokenType.STAR,  "*", null, 1);
        Token minusTok = new Token(TokenType.MINUS, "-", null, 1);

        Expr expr =
                new Expr.Binary(
                        new Expr.Grouping(
                                new Expr.Binary(new Expr.Literal(1.0), plusTok, new Expr.Literal(2.0))
                        ),
                        starTok,
                        new Expr.Grouping(
                                new Expr.Binary(new Expr.Literal(4.0), minusTok, new Expr.Literal(3.0))
                        )
                );

        System.out.println(new RpnPrinter().print(expr));
    }
}